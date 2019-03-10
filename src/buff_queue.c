
#include "buff_queue.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>

static uint64_t get_len(char *buf) {
	uint64_t u;
	memcpy((void *)&u, buf, sizeof(uint64_t));
	return u;
}

static void set_len(char *buf, uint64_t u) {
	memcpy(buf, (void *)&u, sizeof(uint64_t));
}

buff_queue buffq_create(uint64_t buff_size)
{
	buff_queue queue;
	char* buff = (char*)malloc(buff_size);
	if (!buff) {
		return queue;
	}
	queue.data = buff;

	buffq_header* _header = &queue.header;
	_header->buff_size = buff_size;
	_header->rsv_size = 8;
	_header->begin = 0;
	_header->end = 0;
	_header->count = 0;

	return queue;
}

int32_t buffq_enqueue1(buff_queue queue,
					   const sds sbuf)
{
	uint64_t len = sdslen(sbuf);
	if(len == 0) {
		return -1;
	}
	if(buffq_is_full(queue, len)) {
		return -1;
	}

	buffq_header* _header = &queue.header;
	const char* _data = queue.data;
	const char* buffer = *sbuf;
	// 长度字段被分段
	if(_header->end + sizeof(uint64_t) > _header->buff_size) {
		char tmp[16];
		set_len(tmp, len);
		memcpy(_data + _header->end, tmp, _header->buff_size - _header->end);
		memcpy(_data,
			   tmp + _header->buff_size - _header->end,
			   _header->end + sizeof(uint64_t) -_header->buff_size);
		memcpy(_data + _header->end + sizeof(uint64_t) - _header->buff_size, buffer, len);
		_header->end = len + _header->end + sizeof(uint64_t) - _header->buff_size;
		assert(_header->end + _header->rsv_size <= _header->begin);
	} 
	// 数据被分段
	else if(_header->end + sizeof(uint64_t) + len > _header->buff_size){
		set_len(_data + _header->end, len);
		memcpy(_data + _header->end + sizeof(uint64_t),
			   buffer,
			   _header->buff_size - _header->end - sizeof(uint64_t));
		memcpy(_data,
			   buffer + _header->buff_size - _header->end - sizeof(uint64_t),
			   len - (_header->buff_size - _header->end - sizeof(uint64_t)));
		_header->end = len - (_header->buff_size - _header->end - sizeof(uint64_t));
		assert(_header->end + _header->rsv_size <= _header->begin);
	} else {
		set_len(_data + _header->end, len);
		memcpy(_data + _header->end + sizeof(uint64_t), buffer, len);
		_header->end = (_header->end + sizeof(uint64_t) + len) % _header->buff_size;
	}

    _header->count++;
    return 1;
}

int32_t buffq_dequeue1(buff_queue queue,
					   sds* sbuf)
{
	if (!*sbuf) {
		return -1;
	}

	sdsclear(*sbuf);
	if(buffq_is_empty(queue)) {
		return -1;
	}

	buffq_header* _header = &queue.header;
	const char* _data = queue.data;
    //无论成功与否，消息个数都减1
    if(_header->count){
       _header->count--; 
    }

	if(_header->end > _header->begin) {
		assert(_header->begin + sizeof(uint64_t) < _header->end);
		uint64_t len = get_len(_data + _header->begin);
		assert(_header->begin + sizeof(uint64_t) + len <= _header->end);
		sdscpylen(*sbuf, _data + _header->begin + sizeof(uint64_t), len);
		_header->begin += len + sizeof(uint64_t);
	} else {
		// 被分段
		assert(_header->end + _header->rsv_size <= _header->begin);
		uint64_t len = 0;
		uint64_t new_begin = 0;
		char *data_from = NULL;
		char *data_to = NULL;
		assert(_header->begin + 1 <= _header->buff_size);

		// 长度字段也被分段
		if(_header->begin + sizeof(uint64_t) > _header->buff_size) { 
			char tmp[16];
			memcpy(tmp, _data + _header->begin, _header->buff_size - _header->begin);
			memcpy(tmp + _header->buff_size - _header->begin,
				   _data,
				   _header->begin + sizeof(uint64_t) - _header->buff_size);
			len = get_len(tmp);
			data_from = _data + (_header->begin + sizeof(uint64_t) - _header->buff_size); //
			new_begin = _header->begin + sizeof(uint64_t) - _header->buff_size + len;
			assert(new_begin <= _header->end);
		} else {
			len = get_len(_data + _header->begin);
			data_from = _data + _header->begin + sizeof(uint64_t);
			if(data_from == _data + _header->buff_size) {
				data_from = _data;
			}
			if(_header->begin + sizeof(uint64_t) + len < _header->buff_size) { 
				new_begin = _header->begin + sizeof(uint64_t) + len;
			} else { // 数据被分段
				new_begin = _header->begin + sizeof(uint64_t) + len - _header->buff_size;
				assert(new_begin <= _header->end);
			}
		}
		data_to = _data + new_begin;
		_header->begin = new_begin;

		if(data_to > data_from) {
			assert(data_to - data_from == (uint64_t)len);
			sdscpylen(*sbuf, data_from, len);
		} else {
			sdscatlen(*sbuf, data_from, _data - data_from + _header->buff_size);
			sdscatlen(*sbuf + (_data - data_from + _header->buff_size), _data, data_to - _data);
			assert(_header->buff_size - (data_from - data_to) == len);
		}
	}

	return 1;
}

int32_t buffq_enqueue2(buff_queue queue,
					   const sds sbuf1,
					   const sds sbuf2)
{
	if (!sbuf1 || !sbuf2) {
		return -1;
	}

	uint64_t len1 = sdslen(sbuf1);
	uint64_t len2 = sdslen(sbuf2);
	uint64_t len =  len1 + len2;
	if (!queue.data) {
		return -1;
	}
	if(len == 0) {
		return -1;
	}
	if(buffq_is_full(queue, len)) {
		return -1;
	}

	buffq_header* _header = &queue.header;
	const char* buffer1 = sbuf1;
	const char* buffer2 = sbuf2;

	const char* _data = queue.data;
	// 长度字段被分段
	if(_header->end + sizeof(uint64_t) > _header->buff_size) {
		char tmp[16];
		set_len(tmp, len);
		memcpy(_data + _header->end,
			   tmp,
			   _header->buff_size - _header->end);
		memcpy(_data,
			   tmp + _header->buff_size - _header->end,
			   _header->end + sizeof(uint64_t) - _header->buff_size);
		memcpy(_data + _header->end + sizeof(uint64_t) - _header->buff_size,
			   buffer1,
			   len1);
		memcpy(_data + _header->end + sizeof(uint64_t) - _header->buff_size + len1,
			   buffer2,
			   len2);
		_header->end = len + _header->end + sizeof(uint64_t) - _header->buff_size;
		assert(_header->end + _header->rsv_size <= _header->begin);
	}
	// 数据被分段
	else if(_header->end + sizeof(uint64_t) + len > _header->buff_size){
		set_len(_data + _header->end, len);
		if(_header->end + sizeof(uint64_t) + len1 > _header->buff_size) { //buffer1被分段
			memcpy(_data + _header->end + sizeof(uint64_t),
				   buffer1,
				   _header->buff_size - _header->end - sizeof(uint64_t));
			memcpy(_data,
				   buffer1 + _header->buff_size - _header->end - sizeof(uint64_t),
				   len1 - (_header->buff_size - _header->end - sizeof(uint64_t)));
			memcpy(_data + len1 - (_header->buff_size - _header->end - sizeof(uint64_t)),
				   buffer2,
				   len2);
		} else { //buffer2被分段
			memcpy(_data + _header->end + sizeof(uint64_t),
				   buffer1,
				   len1);
			memcpy(_data + _header->end + sizeof(uint64_t) + len1,
				   buffer2,
				   _header->buff_size - _header->end - sizeof(uint64_t) - len1);
			memcpy(_data,
				   buffer2 + _header->buff_size - _header->end - sizeof(uint64_t) - len1,
				   len2 - (_header->buff_size -_header->end - sizeof(uint64_t) - len1));
		}
		_header->end = len - (_header->buff_size - _header->end - sizeof(uint64_t));
		assert(_header->end + _header->rsv_size <= _header->begin);
	} else {
		set_len(_data + _header->end, len);
		memcpy(_data + _header->end + sizeof(uint64_t),
			   buffer1,
			   len1);
		memcpy(_data + _header->end + sizeof(uint64_t) + len1,
			   buffer2,
		       len2);
		_header->end = (_header->end + sizeof(uint64_t) + len) % _header->buff_size;
	}

    _header->count++;

    return 1;
}

int32_t buffq_dequeue2(buff_queue queue,
					   sds* sbuf1,
			           sds* sbuf2)
{
	if (!*sbuf1 || !*sbuf2) {
		return -1;
	}

	sdsclear(*sbuf1);
	sdsclear(*sbuf2);
	if(buffq_is_empty(queue)) {
		return -1;
	}

	uint64_t len1 = sdsavail(*sbuf1);
	if (len1 == 0) {
		return -1;
	}
	buffq_header* _header = &queue.header;
	const char* _data = queue.data;
    if(_header->count) {
       _header->count--; 
    }

	if(_header->end > _header->begin) {
		assert(_header->begin + sizeof(uint64_t) < _header->end);
		uint64_t len = get_len(_data + _header->begin);
		assert(_header->begin + sizeof(uint64_t) + len <= _header->end);

		// ensure fmalloc length of *sbuf1 > 32
		if(len1 > len) {
			sdscpylen(*sbuf1, _data + _header->begin + sizeof(uint64_t), len);
		} else {
			uint64_t len2 = len - len1;
			sdscpylen(*sbuf1, _data + _header->begin + sizeof(uint64_t), len1);
			sdscpylen(*sbuf2, _data + _header->begin + sizeof(uint64_t) + len1, len2);
		}
		_header->begin += len + sizeof(uint64_t);
	} else {
		// 被分段
		assert(_header->end + _header->rsv_size <= _header->begin);
		uint64_t len = 0;
		uint64_t new_begin = 0;
		char *data_from = NULL;
		char *data_to = NULL;
		assert(_header->begin + 1 <= _header->buff_size);
		// 长度字段也被分段
		if(_header->begin + sizeof(uint64_t) > _header->buff_size) { 
			char tmp[16];
			memcpy(tmp,
				   _data + _header->begin,
				   _header->buff_size -_header->begin);
			memcpy(tmp + _header->buff_size - _header->begin,
				   _data,
				   _header->begin + sizeof(uint64_t) - _header->buff_size);
			len = get_len(tmp);
			data_from = _data + (_header->begin + sizeof(uint64_t) - _header->buff_size); //
			new_begin = (_header->begin + sizeof(uint64_t) - _header->buff_size) + len;
			assert(new_begin <= _header->end);
		} else {
			len = get_len(_data + _header->begin);
			data_from = _data + _header->begin + sizeof(uint64_t);
			if(data_from == _data + _header->buff_size) {
				data_from = _data;
			}
			if(_header->begin + sizeof(uint64_t) + len < _header->buff_size) { 
				new_begin = _header->begin + sizeof(uint64_t) + len;
			} else { // 数据被分段
				new_begin = _header->begin + sizeof(uint64_t) + len - _header->buff_size;
				assert(new_begin <= _header->end);
			}
		}
		data_to = _data + new_begin;
		_header->begin = new_begin;

		if(data_to > data_from) {
			assert(data_to - data_from == (uint64_t)len);
			if(len1 > len) {
				sdscpylen(*sbuf1, data_from, len);
			} else {
				uint64_t len2 = len - len1;
				sdscpylen(*sbuf1, data_from, len1);
				sdscpylen(*sbuf2, data_from + len1, len2);
			}
		} else {
			assert(_header->buff_size - (data_from - data_to) == len);
			if(len1 > len) {
				sdscatlen(*sbuf1, data_from, _data + _header->buff_size - data_from);
				sdscatlen(*sbuf1 + (_data + _header->buff_size - data_from), _data, data_to - _data);
			} else if(len1 > _data - data_from + _header->buff_size) { //buffer1被分段
				uint64_t len2 = len - len1;
				sdscatlen(*sbuf1, data_from, _data + _header->buff_size - data_from);
				sdscatlen(*sbuf1 + (_data + _header->buff_size - data_from),
					   	  _data,
					   	  len1 - (_data + _header->buff_size - data_from));
				sdscpylen(*sbuf2, data_from + sdslen(*sbuf1) - _header->buff_size, len2);
			} else { //buffer2被分段
				sdscpylen(*sbuf1,
					      data_from,
					      len1);
				sdscatlen(*sbuf2,
					   	  data_from + len1,
					   	  _data + _header->buff_size - data_from - len1);
				sdscatlen(*sbuf2 + (_data + _header->buff_size - data_from - len1),
					   	  _data,
						  len - (_data - data_from + _header->buff_size));
			}
		}
	}

	return 1;
}

int32_t buffq_is_full(buff_queue queue, uint64_t len)
{
	if( len == 0) {
		return -1;
	}

	const buffq_header* _header = &queue.header;
	if(_header->end == _header->begin) {
		if(len + sizeof(uint64_t) + _header->rsv_size > 
			_header->buff_size) {
			return 0;
		}
		return -1;
	}

	if(_header->end > _header->begin) {
		assert(_header->begin + sizeof(uint64_t) < _header->end);
		uint64_t remain_size = _header->buff_size - _header->end + _header->begin;
		return (remain_size < sizeof(uint64_t) + len + _header->rsv_size);
	}

	assert(_header->end + _header->rsv_size <= _header->begin);
	return (_header->begin - _header->end < 
			sizeof(uint64_t) + len + _header->rsv_size);
}

int32_t buffq_is_empty(buff_queue queue)
{
	const buffq_header* _header = &queue.header;
	return _header->begin == _header->end;
}
