-- HashKey --
hashkey ������ ���ؼ� CreateHashKey�Լ��� ����Ѵ�.
hashkey�� ������ �� ������Ʈ�� ���ڿ��� client�� cp���� ��û�ϴ� raw url�� ����Ѵ�.
"raw url"�̶� ���� hot number�� acl�� �̿��� �����ϱ� ������ url�� ���Ѵ�.


-- Bill Info --
CreateBillInfoKey() �Լ��� �����Ͽ� key�� �����Ѵ�.
key�� 10�ڸ��� sequence �� ���� �����Ѵ�.
CreateBillInfoKey() �Լ��� �ſ� ���� Ȯ���� ���� key ���� �����Ǵ� �������� �ִ�.

-- �Ķ���� ��� --
[subfix "_"]
Ŭ���� ��������� port ��� ������ �ְ�, ����Լ��� �Ķ���ͷ� port ��� ������ �޾Ƶ鿩�� �Ҷ�,
����Լ� �Ķ������ port �� ������� port�� scope �� ������ ������ ��ȸ�ϱ� ����
xxxx_ �� ���� _ �̶�� subfix, ���� ��� port_ �� ���� �Ķ���� ���ڸ� ����ϸ�, scope�� ������ �ʴ�
��쿡�� _ ��� subfix�� ������ �ʴ´�.

[subfix _HandOver]
�Լ�ȣ���߿��� �ܺο��� Heap�� �����Ҵ��ϰ� �Լ� ������ ������ �߻��ϴ� ��찡 �ִ�.
���� ���� ȣ��� �Լ��� �Ķ���ͷ� Heap�� ���� �����͸� �޾Ƽ� ���������� �˾Ƽ� ������ �� ��, _HandOver�� �Ķ���͸� ���δ�.
��, sendData_HandOver�� ���� xxxxx_HandOver ��� ������ ����Ѵ�.


-- msgwatch, mwatch --
CommonSocket.cpp CommonSocket.h mwatchcli.cpp mwatchcli.h ȭ�� ���� ��ü���� ������ �ҽ��̴�.
msgwatch.h �� �Ʒ��� ���� �����Ǵ� ȭ���̴�.
cat CommonSocket.h mwatchcli.h > msgwatch.h


