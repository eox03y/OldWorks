**���� ó�� �׽�Ʈ
  #[KUN] python stattest.py k_stat.cfg [n]
  #[ME] python stattest.py stat.cfg [n]
  [n] �� �׽�Ʈ ����.
  �־��� stat.cfg ���� comment out �� ���� ó������ ��� �����Ͽ� �׽�Ʈ.

kunclient.py
	KUN ������ ����. �⺻ ��ɸ� ����. 
	�ѹ���  Request ������ ���� ����.
	socket �� blocking �Ǵ� non-blocking���� �����ϱ� ���ؼ���  �ҽ����� ���� �ʿ�.
	REQ header�� ������ ������� ���� (�ҽ� ������ ����) ���� ���̽��� �׽�Ʈ �� �� �ִ�.

meclient.py
	ME ������ ����. �⺻ ��ɸ� ����. 
	�ѹ���  Request ������ ���� ����.
	
pipelinging.py
pipe2.py
	PAS2006������ ���������̴� ����� �׽�Ʈ�ϱ� ���� �ܸ� ����.

���丮 - chunk
	PAS2006 ������ Chunked data (CP RESP body)�� ó�� ��� �� ���� ��Ȳ ó���� Ȯ���ϱ�
	���ؼ� ���� ������, CP web server�� �䳻���� Chunked ������ CP response�� �����Ѵ�.
	���� ����:
		Chunked ���̺��� ������ ��ų� ª�� ���.
		Chunked �̸鼭 Content-Length�� ���Ե� ���.
		Content-Length ������ ������ ��ų� ª�� ���.
		
AcceptWithDelay.py
	���� �Ķ���ʹ� ������Ʈ ��ȣ�� delay_sec�� �޴´�.  
	acceptWithDelay.py �� ������ ������ ���� ����Ѵ�. �ٸ� �׻� �� ���� ������ �Ѵ�.
	���ÿ� �� Ŭ���̾�Ʈ�� ó���Ѵ�. tcp server�� �����, listen() �ϴٰ�, accept()�� 2~3 �� �Ŀ� 
	accept()�Ŀ� read()�ϰ�, ������ ���ǵ� http mesg�� send()�Ѵ�.

	���ǵ� �޼��� = \
	"""\
	HTTP 200 OK\r\n
	Content-Length: 20\r\n
	\r\n
	<html><body>Good</body></html>
	"""


complog.py
	1) ���αװ��
	python complog.py -s oldfile newfile
	�ϰ� �Ǹ� �� ������ �α׸� ���� ���ϰ� ���� URL��û�ε� ������ �ٸ���� �ٸ� �׸�(field)��
	���� ������ݴϴ�.

	2) ���ݷαװ��
	python complog.py -m oldfile newfile

	python complog.py �Ͻø� ���� ������ ���� �� �ֽ��ϴ�.


ftpServer.py
	������ �� �ӽſ��� python ftpServer.py PORT �� �����Ͻø� �˴ϴ�.
	�������� ����Ǵ� ������ Client���� �ø��� �����̸��� �����մϴ�. 


ftpClient.py
	Ŭ���̾�Ʈ�� �� �ӽſ��� python ftpClient Host PORT Filename
	�Ͻø� ��ǥ HOST, PORT�� Filename �� ����˴ϴ�. �����ϸ� Success��� �޼����� ���ϸ�, 
	����� ��ϴ�.;;
	ftp ���α׷� �Ѵ� �Ѵ� python ����ȭ��.py �Ͻø� ������ ���Ǽ� �ֽ��ϴ�.


DnsCache.py 
DNSClient.py

	1. ���
	DNS Query Program �Դϴ�. Query �ϴµ� 0.8���̻� �ɸ��� 0.0.0.0 �� Return �մϴ�. :)
					       0.8������ �ɸ��� IP�� Return�մϴ�.
	2. ����
	 python DnsCache.py ��Ʈ��ȣ
	 
	3. Test Driven...
	�׽�Ʈ�� ���ؼ� DNSClient.py �� ��������ϴ�.
	http://anydic.com/svn/PAS2006/trunk/src/emul/DNSClient.py
	�������
	python DNSClient.py ��ǥȣ��Ʈ ��ǥ��Ʈ

	�Ʒ�ó�� �˴ϴ�.
	 
	python DNSClient.py localhost 3333
	connected to server
	----------------------------------------------------------------------
	anydic.com :Response from Server-> 218.153.100.91
	----------------------------------------------------------------------




���丮 - benchmark
	PAS2006 �� ���� ���� ������ ����.
	urlList.txt�� �ִ� URL ����� �������� ���� ���� �ܸ��� �����ϸ�, PAS ������ ���ϸ� ���ϰ�,
	�� ��� ��踦 ����Ѵ�.

���丮 - simplebench
	PAS2006 �� ���� ���� ������ ����. benchmark/* �� �ణ ������ ��.
	�ܸ��� ������ �ֱ������� ������ų �� ������ �ܸ� thread ���� ��⵿(���� �� �ٽ� ����)���� �ʰ�,
	�� �����常 �߰��ϴ� ������� ������ ���̴�.
   
=== santaHttpServer.py  ,   santa.acl   ===

�����ȣ == MIN : �ܸ��� browser�� ��� �ö���� ��ȣ.�����ȣ�� 01690xx, 01691xx �뿪�Դϴ�.
MDN: ���� ��ȣ, ����ڰ� �ڱ��ȣ�� �˰� �ִ� �Ϲ� ��ȣ.
SANTA: �����ȣ (MIN��ȣ)�� �ش��ϴ� MDN�� �����ϰ�, ��ȸ�� �� �ֵ��� �����ϴ� ����.

ME �ܸ��׽�Ʈ�� ������ �� ��κ��� ME�ܸ��� �����ȣ�̱� ������ santa ��ȸ�� �ʿ��մϴ�.

�׷��� TB������ ��� SANTA�� ������ �� ����,
TB SANTA�� ������, ���ʿ� ����Ǿ� �ִ� MIN��ȣ�� Ư�� �뿪�̶�,
�츮�� �ܸ��� �׽�Ʈ �ϴµ� ����ϱ� ��ƴ�.

�׷��� �츮�� ���� santa emul �ִ�.

=== santa emul ����.
http://zetamobile.com/svn/PAS2006/trunk/src/emul/santaHttpServer.py
http://zetamobile.com/svn/PAS2006/trunk/src/emul/santa.acl    -- santa ���� ��ȣ,  MIN <--> MDN ���� ����Ÿ �� ��� �ִ�.

���� : �Ʒ��� ���� �⵿ (port 30001)�ϰ�, ME/KUN �� ���Ǳ׿��� santa ��Ʈ �� ��ȣ�� �°� �����Ͽ��� �Ѵ�.
# python santaHttpServer.py 30001  santa.acl


== santa.acl ����
# SANTA ���� ��ȣ ���

SVCID mapexam
ID pas
PASSWORD pas

#  MIN(virtual)   IMSI          MDN
01690787300 420001690787300 01032166366
01691971624  420001691971624 01000001624
1691971624  420001691971624 01000001624
1690054236 420001690054236 01000004236
01690054236 420001690054236 01000004236

