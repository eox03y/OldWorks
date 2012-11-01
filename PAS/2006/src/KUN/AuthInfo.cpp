#include "AuthInfo.h"

AuthInfoMessageBlock::AuthInfoMessageBlock(AUTH::RequestBody *body, UserInfo *userinfo)
{
	memcpy( &reqBody, body, sizeof(reqBody) );
	memcpy( &userInfo, userinfo, sizeof(userInfo) );

	int total = sizeof(reqBody) + sizeof(userInfo);
	size( total );

	ACE_ASSERT(static_cast<int>(space()) >= total);

	// ������ ACE_Message_Block �� �뵵�� ���ϸ�,
	// �޽��� ���� ������ ī���Ͽ� �����Ͽ��� ������
	// ���� ����� �޸� ���縦 ������ �ʿ䰡 ����.
	// �׷��Ƿ� �� ����� ���� ������ if(mesg->length()==0) �� ���ϱ� ���Ͽ�
	// Write Pointer �� ������Ų��.
/*
	memcpy((void*)wr_ptr(), (const void*)&reqBody, sizeof(reqBody));
	wr_ptr( sizeof(reqBody) );

	memcpy((void*)wr_ptr(), (const void*)&userInfo, sizeof(userInfo));
	wr_ptr( sizeof(userInfo) );
*/

	wr_ptr( total );
}

AuthInfoMessageBlock::~AuthInfoMessageBlock()
{
}

