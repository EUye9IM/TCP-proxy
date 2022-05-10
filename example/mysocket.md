# mysocket.hpp ʹ��˵��
## ����
`mysocket.hpp`�Ƕ�socket������غ����ķ�װ��ֻ����socket�ĳ�ʼ�����֡�

����ʵ�ֶ���hpp�ļ�����ɣ�ֱ��include���ɣ������ռ�Ϊ Anakin::

## �����
����Ϊ����˳�ʼ�����룺

```C++
Anakin::Socket_Accept server_socket(AF_INET, SOCK_STREAM, 0);

// ���õ�ַ����
int opt = 1;
server_socket.setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

// ��������Ϊ��������ʽ
server_socket.SetSocketBlockingEnable(false);

server_socket.Bind((struct sockaddr*)&address, sizeof(address));
server_socket.Listen(3);
cout << "listen..." << endl;
```

���ڽ����˴����װ��������������˳�����


## �ͻ���
����Ϊ�ͻ��˳�ʼ�����룺

```C++
Anakin::Socket_Connect client_socket(AF_INET, SOCK_STREAM, 0);

// ��������Ϊ��������ʽ
client_socket.SetSocketBlockingEnable(false);

struct sockaddr_in client_addr;

int myport = 80;
// ��� myport �ڲ���������
if (myport != -1) {
    // bind client�˿�
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(myport);
    client_socket.Bind((struct sockaddr *)&client_addr, sizeof(client_addr));
}

// ��������ʽ����
string ip = "192.168.80.230";
int port = 1024;
int ret = client_socket.Connect(ip, port, false);

```

����connect�����������������Ƿ�������Ĭ��Ϊtrue�����ڷ����������Σ���Ҫ��Ϸ���ֵ��errno�����жϡ�


## ���ڷ������µ�����
socket�ಢû�жԷ������ĺ������з�װ��ֻ���ṩ��getfd������ȡ����������������Զ���ɣ�����read,write,select��

��Ȼ����Ҳ����ʹ��setfd����socket�������������Ǵ�ʱ�������ٻ�ȡ���ӵĵ�ַ�����ù�����Ч

## Socket_Accept��˵��
����Socket_Accept�࣬����ά��һ��listenfd�������ӣ���accept���µ�fd�᷵�أ����洢�����е�map�У�������Բ鿴

## �����������Ĺر�
Ŀǰ�Ѿ����������������ر�������������Socket_Accept�е����ӵĿͻ���socketҲ�������������йرմ���

## ����
ͷ�ļ�������������������checkerror�������ж�ĳ������ֵ�Ƿ�<0�����ǣ���perror��ӡ���˳���
���������ȡ����ip��ַ���ж��Ƿ��Ǳ���������ַ�ĺ�������Ϊ��ʷ��������
