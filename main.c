#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/neutrino.h>

#define BUFFERSIZE 50

int chid;	    // идентификатор канала

void *server()    // поток-сервер
{
	int rcvid;
	int i=0;
	_int8 code;
	int value;
	char receive_buf[BUFFERSIZE], reply_buf[BUFFERSIZE];
	printf("# Server thread:  Channel creating ...");
	// создание канала с опциями по умолчанию и запись в chid номера канала
	chid=ChannelCreate(0);
	if (chid<0)
	{
	   perror("Server error");
	   exit(EXIT_FAILURE);
	}	
	printf("# CHID = %d\n", chid);
	printf("# Server thread: Listen to channel %d\n", chid);
	while (1)	// сервер работает в цикле
	{
	  // принимаем сообщение из канала с номером chid в буфер receive_buf
	  // в rcvid записывается идентификатор полученного сообщения
		rcvid=MsgReceive(chid, &receive_buf, sizeof(receive_buf), NULL);
		if (rcvid>0)	// получили обычное сообщение
		{
		    printf("# Server thread: message <%s> has received.\n", receive_buf);
		    strcpy(reply_buf, "Answer from server");
		    printf("# Server thread: answering with <%s> (Status=%d).\n", reply_buf, i);
	// отправляем ответ (буфер reply_buf) по номеру полученного сообщения (rcvid)
	// второй параметр (в данном случае переменная i)
	// статус ответа, обрабатывается клиентом.
		    MsgReply(rcvid, i, &reply_buf, sizeof(reply_buf));
		     i++;
		}
		if (rcvid==0)	// получили импульс
		{
		    code=receive_buf[4]; // 4-ый байт - это код импульса (по структуре _pulse)
		    // байты с 8 по 11 - это данные, соберем их в переменную value
		    value=receive_buf[11];
		    value<<=8;	value+=receive_buf[10];
	                value<<=8;	value+=receive_buf[9];
		    value<<=8;	value+=receive_buf[8];
		    printf("# Server thread: received pulse - code=%d, value=%d.\n", code, value);
		}
	}
}

void *client(void *parametr)	// поток-клиент
{
	int coid, status;
	_int8 code;
	int value;
	pid_t PID;
	pthread_t client;
	char send_buf[BUFFERSIZE], reply_buf[BUFFERSIZE];
	PID=getpid();
	client=pthread_self(); // получаем идентификатор потока-клиента
	printf("> Client thread %d: connecting to channel ... ", client);
	// создаем соединение с каналом на текущем узле (0)
	// канал принадлежит процессу с идентификатором PID
	// номер канала - chid
	// наименьшее значение для COID - 0
	// флаги соединения не заданы - 0
	coid=ConnectAttach(0, PID, chid, 0, 0);
	// в coid записан идентификатор соединения или ошибочное значение меньше нуля
	if (coid<0)
	{
	   perror("Client error");
	   exit(EXIT_FAILURE);
	}
	printf("COID = %d\n", coid);
	if (client%2==0)	// четные потоки будут отправлять сообщения
	{
	    strcpy(send_buf, "It's very simple example");
	    printf("> Client thread %d: sending message <%s>.\n", client, send_buf);
	    // отправляем сообщение из буфера send_buf в соединение coid
	    // ответ принимаем в буфер reply_buf и статус записывается в переменную status
	    status=MsgSend(coid, &send_buf, sizeof(send_buf), &reply_buf, sizeof(reply_buf));
	    printf("> Client thread %d: I have replied with massage <%s> (status=%d).\n", client, re-ply_buf, status);
	}
	else
	{  // нечетные потоки будут отправлять импульсы
	    code=20;
	    value=12345;
	    printf("> Client thread %d: sending pulse - code=%d, value=%d.\n", code, value);
	    // посылаем импульс в соединение coid
	    // приоритет импульса 20
	    // код - code, данные - value
	    MsgSendPulse(coid, 20, code, value);
	}
	// разрываем соединение coid
	ConnectDetach(coid);
	printf("> Client thread %d:  Good bye.\n", client);
	pthread_exit(NULL);
}



int main()
{
	pthread_t client_tid1, client_tid2;
	printf("Main thread: starting Server & Clients ...\n");
	// создаем потоки сервера и двух клиентов
	pthread_create(NULL, NULL, server, NULL);
	sleep(1);
	pthread_create(&client_tid1, NULL, client, NULL);
	pthread_create(&client_tid2, NULL, client, NULL);
	printf("Main thread: waiting for child threads exiting ...\n");
	// ждем их завершения
	pthread_join(client_tid1, NULL); pthread_join(client_tid2, NULL);
	printf("Main thread: the end.\n");
	return EXIT_SUCCESS;
}
