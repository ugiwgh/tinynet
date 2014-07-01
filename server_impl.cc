#include "server_impl.h"
#include <stdio.h>
#include <string.h>
#include <thread>

#define CC_CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)

const unsigned int Server_Impl::max_buffer_size_ = 1024;

Server_Impl::Server_Impl( Reactor* __reactor,const char* __host /*= "0.0.0.0"*/,unsigned int __port /*= 9876*/ )
	: Event_Handle_Srv(__reactor,__host,__port) 
{
	auto __thread = std::thread(CC_CALLBACK_0(Server_Impl::_work_thread,this));
	__thread.detach();
}

void Server_Impl::on_connected( int __fd )
{
	printf("on_connected __fd = %d \n",__fd);
	connects_[__fd] = new easy::EasyRingbuffer<unsigned char,easy::alloc>(max_buffer_size_);
}

void Server_Impl::on_read( int __fd )
{
	if(0)
	{
		_read_directly(__fd);
	}
	else
	{
		_read(__fd);
	}
}

void Server_Impl::_read_directly( int __fd )
{
	if (0)
	{
		//	just transform data
		char __buf[max_buffer_size_] = {0};
		int __recv_size = Event_Handle_Srv::read(__fd,__buf,max_buffer_size_);
		if (0)
		{
			printf("on_read data is %s,length is %d\n",__buf,__recv_size);
		}
		if(-1 != __recv_size)
		{
			write(__fd,__buf,__recv_size);
		}
	}
	else
	{
		//	read head first.and then read the other msg.just a test code
		static const int __head_size = 12;
		unsigned long __usable_size = 0;
		int __packet_length = 0;
		int __log_level = 0;
		int __frame_number = 0;
		int __head = 0;
		unsigned int __guid = 0;
		unsigned char __packet_head[__head_size] = {};
		int __recv_size = 0;
		while (true)
		{
			_get_usable(__fd,__usable_size);
			if(__usable_size >= __head_size)
			{
				__recv_size = Event_Handle_Srv::read(__fd,(char*)&__packet_head,__head_size);
				if(__head_size != __recv_size)
				{
					printf("error: __recv_size = %d\n",__recv_size);  
					return ;
				}
				memcpy(&__packet_length,__packet_head,4);
				memcpy(&__head,__packet_head + 4,4);
				memcpy(&__guid,__packet_head + 8,4);
				__log_level = (__head) & 0x000000ff;
				__frame_number = (__head >> 8);
				write(__fd,(char*)__packet_head,__recv_size);
			}
			else
			{
				return;
			}
			_get_usable(__fd,__usable_size);

			if(__usable_size >= __packet_length)
			{
				char __buf[max_buffer_size_] = {0};
				int __recv_size = Event_Handle_Srv::read(__fd,__buf,__packet_length);
				write(__fd,__buf,__recv_size);
			}
			else
			{
				return;
			}
		}
	}
}

void Server_Impl::_read( int __fd )
{
	//	the follow code is ring_buf's append function actually.
	unsigned long __usable_size = 0;
	_get_usable(__fd,__usable_size);
	int __ring_buf_tail_left = connects_[__fd]->size() - connects_[__fd]->wpos();
	if(__usable_size <= __ring_buf_tail_left)
	{
		Event_Handle_Srv::read(__fd,(char*)connects_[__fd]->buffer() + connects_[__fd]->wpos(),__usable_size);
		connects_[__fd]->set_wpos(connects_[__fd]->wpos() + __usable_size);
	}
	else
	{	
		//	if not do this,the connection will be closed!
		if(0 != __ring_buf_tail_left)
		{
			Event_Handle_Srv::read(__fd,(char*)connects_[__fd]->buffer() +  connects_[__fd]->wpos(),__ring_buf_tail_left);
			connects_[__fd]->set_wpos(connects_[__fd]->size());
		}
		int __ring_buf_head_left = connects_[__fd]->rpos();
		int __read_left = __usable_size - __ring_buf_tail_left;
		if(__ring_buf_head_left >= __read_left)
		{
			Event_Handle_Srv::read(__fd,(char*)connects_[__fd]->buffer(),__read_left);
			connects_[__fd]->set_wpos(__read_left);
		}
		else
		{
			Event_Handle_Srv::read(__fd,(char*)connects_[__fd]->buffer(),__ring_buf_head_left);
			connects_[__fd]->set_wpos(__ring_buf_head_left);
		}
	}
}

void Server_Impl::_work_thread()
{
	static const int __head_size = 12;
	while (true)
	{
		for (std::map<int,ring_buffer*>::iterator __it = connects_.begin(); __it != connects_.end(); ++__it)
		{
			if(__it->second)
			{
				while (!__it->second->read_finish())
				{
					int __packet_length = 0;
					int __log_level = 0;
					int __frame_number = 0;
					unsigned char __packet_head[__head_size] = {};
					int __head = 0;
					unsigned int __guid = 0;
					if(!__it->second->pre_read((unsigned char*)&__packet_head,__head_size))
					{
						//	not enough data for read
						break;
					}
					memcpy(&__packet_length,__packet_head,4);
					memcpy(&__head,__packet_head + 4,4);
					memcpy(&__guid,__packet_head + 8,4);
					if(!__packet_length)
					{
						printf("__packet_length error\n");
						break;
					}
					__log_level = (__head) & 0x000000ff;
					__frame_number = (__head >> 8);
					char __read_buf[max_buffer_size_] = {};
					if(__it->second->read((unsigned char*)__read_buf,__packet_length + __head_size))
					{
						write(__it->first,__read_buf,__packet_length + __head_size);
					}
					else
					{
						break;
					}
				}
			}
		}
	}
}

void Server_Impl::on_disconnect( int __fd )
{
	for (std::map<int,ring_buffer*>::iterator __it = connects_.begin(); __it != connects_.end(); ++__it)
	{
		if (__it->second)
		{
			delete __it->second;
			__it->second = NULL;
			connects_.erase(__it);
			return;
		}
	}
}

