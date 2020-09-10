#ifndef __TRANSPORT_INCLUDE_H__
#define __TRANSPORT_INCLUDE_H__


class Transport {
public:
	virtual int Init() = 0;
	virtual int Destory();
	virtual  int SendMessage();
};


#endif // __TRANSPORT_INCLUDE_H__