#ifndef __AUTH_INFO_H__
#define __AUTH_INFO_H__

/**
@brief AuthAgent�� ���ο� Job�� �߰��� �� ���

userInfo = UserInfo ����
reqBody  = Auth ������ ���� ������ ���� ������
respBody = Auth �������� ���� ���� ������ ����ȴ�.(AuthAgent �� ����Ѵ�.)
*/

#include <ace/Message_Block.h>
#include "AuthTypes.h"
#include "UserInfo.h"

class AuthInfo
{
public:
	AuthInfo() {}
	virtual ~AuthInfo() {}

	AUTH::ResponseBody respBody;
	AUTH::RequestBody reqBody;
	UserInfo userInfo;
};

class AuthInfoMessageBlock : public ACE_Message_Block
{
	// Member Functions
public:
	AuthInfoMessageBlock(AUTH::RequestBody *body, UserInfo *userinfo);
	virtual ~AuthInfoMessageBlock();

private:
protected:

	// Member Variables
public:
	AUTH::ResponseBody respBody;
	AUTH::RequestBody reqBody;
	UserInfo userInfo;

private:
protected:

};

#endif
