#include "KtfInfo.h"
#include "ResponseBuilder.h"

#include "KunHandler.h"

KunHandler::KunHandler(ReactorInfo* rInfo) : ClientHandler(rInfo)
{

}

KunHandler::~KunHandler(void)
{
}

int KunHandler::procACL( Transaction* tr )
{
	HTTP::Request* pRequest = tr->getRequest();
	host_t destProxyHost;
	int destProxyPort;
	AclResult aclResult = applyACL(pRequest, destProxyHost, destProxyPort);

	if(aclResult == AR_CHANGE_PROXY)
	{
		// browser type �� �߸��̸鼭 ��Ƽ �������� ��쿡�� ������ ������ �����Ѵ�.
		/*-- Browser Type Check --*/
		KtfBrowserType browseType =KtfInfo::checkBrowserType(tr->browser);
		if (browseType == BROWSER_TYPE_ME)
		{
			PAS_INFO1("Wrong Browser: BROWSER_TYPE_ME %s", getMyInfo());
			PHTR_INFO1("Wrong Browser: BROWSER_TYPE_ME %s", getMyInfo());
			ResponseBuilder::InvalidProxy_toME( tr->getResponse() );
			tr->setErrorRespCode(RESCODE_WRONG_PROXY); // Over10 �α� ����� ���� ���� �ڵ�
			afterCpTransaction(tr);
			return -1;
		}

		else
		{
			// PROXY ���� ��û
			// CLIENT���� PROXY ������ �˸��� ������������ �۽�
			tr->setErrorRespCode(RESCODE_MULTIPROXY);
			ResponseBuilder::ProxyChange(tr->getResponse(), destProxyHost, destProxyPort);
			afterCpTransaction(tr);
			return -1;
		}
	}
	else if (aclResult == ACL_DNS_APPLIED)
	{
		tr->setCpConnInfo_apply_ACL(destProxyHost.toStr(), destProxyPort);		
	}

	return 0;
}

int KunHandler::browserTypeCheck( Transaction* tr )
{
	KtfBrowserType	browseType = KtfInfo::checkBrowserType(tr->browser);
	if (browseType == BROWSER_TYPE_ME)
	{
		PAS_INFO1("Wrong Browser: BROWSER_TYPE_ME %s", getMyInfo());
		PHTR_INFO1("Wrong Browser: BROWSER_TYPE_ME %s", getMyInfo());
		ResponseBuilder::InvalidProxy_toME( tr->getResponse() );
		tr->setErrorRespCode( RESCODE_WRONG_PROXY ); // Over10 �α� ����� ���� ���� �ڵ�
		afterCpTransaction(tr);
		return -1;
	}
	else if (browseType == BROWSER_TYPE_OTHER)
	{
		PAS_INFO1("Wrong Browser: BROWSER_TYPE_OTHER %s", getMyInfo());
		PHTR_INFO1("Wrong Browser: BROWSER_TYPE_OTHER %s", getMyInfo());
		ResponseBuilder::InvalidAgent( tr->getResponse() );
		tr->setErrorRespCode( RESCODE_WRONG_BROWSER ); // Over10 �α� ����� ���� ���� �ڵ�
		afterCpTransaction(tr);
		return -1;
	}

	return 0;
}
