#include "server_interface.h"
#include "ui_server_interface.h"

Server_Interface::Server_Interface(QWidget *parent): QWidget(parent), ui(new Ui::Server_Interface)
{
    ui->setupUi(this);

    server = new QTcpServer(this);

    connect(ui->pushButton_startServer, &QPushButton::clicked, this, &Server_Interface::ServerOnOrOff);
}

Server_Interface::~Server_Interface()
{
    delete ui;
}

void Server_Interface::logMessage(const QString &msg)
{
    ui->plainTextEdit_serverLog->appendPlainText(msg + QLatin1Char('\n'));
}

void Server_Interface::ServerOnOrOff()
{
    if(server->isListening())
    {
        QString message = "Server disconnected!";
        logMessage(message);
        qDebug() << message;
        server->close();
        ui->pushButton_startServer->setText("Start Server");

//        discardAllClients();
    }
    else
    {
        if(!server->listen(QHostAddress::Any, 9000))
        {
            QString message = "Could not connect to host!";
            ui->pushButton_startServer->setText("Start Server");
            qDebug() << message;
            logMessage(message);
            return;
        }
        QString message = "Server connected and listening....";
        ui->pushButton_startServer->setText("Stop Server");
        qDebug() << message;
        logMessage(message);

        connect(server, SIGNAL(newConnection()), this, SLOT(newConnectionMade()));
    }
}

void Server_Interface::newConnectionMade()
{
    serverSocket = new QTcpSocket();

    serverSocket = server->nextPendingConnection();

    connect(serverSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    connect(serverSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    client_list.insert(serverSocket);
    QString message = "A new client has connected to the server!";
    logMessage(message);

    qDebug() << "Socket Descriptor connected: " << serverSocket->socketDescriptor();
}

void Server_Interface::onReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());

    QString signal;
    int command = 0;

    QByteArray data = clientSocket->readAll();
    qDebug() << "Data In: " << data;

    QStringList dataRead = QString(data).split(":");
    qDebug() <<"Read Data: " << dataRead;

    signal = dataRead[0];

    if(signal == "LOGIN_CHECK")
        command = 1;

    else if (signal == "GETACTIVEUSERS")
        command = 2;

    else if (signal == "PRIVMES")
        command = 3;

//    else if (signal == "PRIV_CHAT_REQ")
//        command = 4;
    else if (signal == "REGISTER")
        command = 5;

    switch(command)
    {
    case 1:
    {
        checkLoginCreds(dataRead[1],dataRead[2]);
        break;
    }
    case 2:
    {
        sendActiveUsersList();
        break;
    }
    case 3:
    {
        recievePrivateMessage(dataRead[1],dataRead[2],dataRead[3]);
        break;
    }
//    case 4:
//    {
//        privateChatClientCheck(dataRead[1],dataRead[2]);
//        break;
//    }


    }
}



void Server_Interface::checkLoginCreds(QString &username, QString &password)
{
    QString uname = username;
    QString pword = password;
    connOpen();
    QSqlQuery qry;
    QString message = "Checking Username: " + uname;
    qDebug() << message;
    logMessage(message);

    if (qry.exec("select username,pword from Clients where username='"+uname+"'"))
    {
        if(qry.next())
        {
            if(qry.value(0)==uname && qry.value(1)==pword)
            {
                QString message = "Username and password match!";
                qDebug() << message;
                logMessage(message);
                loginSuccess(uname,pword);
                connClose();
            }
            else
            {
                QString message = "Login failed!";
                qDebug() << message;
                logMessage(message);
                loginFail(username);
                connClose();
            }
        }
    }
}

void Server_Interface::loginSuccess(QString &username, QString &password)
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());

    QString uname = username;
    QString pword = password;
    QString status = "true";

    QByteArray ba;

    ba.append("LOG_SUC:" + uname.toUtf8() + ":" + status.toUtf8());

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            clientSocket->write(ba);
            QString message = "Writing data: " + ba + " to user: " + uname + " with password: " + pword;
            qDebug() << message;
            logMessage(message);

            onlineUsers.append({uname,clientSocket});
            qDebug() << "Online Users: " << onlineUsers;

            setUserOnline(uname);
        }
    }
}

void Server_Interface::loginFail(QString &creds)
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());

    QString cred = creds;
    QString status = "fail";

    QString response = "LOG_FAIL:";

    QByteArray ba;

    ba.append(response.toUtf8() + cred.toUtf8() + ":" + status.toUtf8());

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            clientSocket->write(ba);
            QString message = "Writing data: " + ba + " to user/password: " + cred;
            qDebug() << message;
            logMessage(message);
        }
    }
}

void Server_Interface::loginDuplicate(QString &creds)
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());

    QString cred = creds;
    QString status = "duplicate";

    QString response = "LOG_DUP:";

    QByteArray ba;

    ba.append(response.toUtf8() + cred.toUtf8() + ":" + status.toUtf8());

    for(auto i : client_list)
    {
        if (i== clientSocket)
        {
            clientSocket->write(ba);
            QString message = "Writing data: " + ba + " to user/password: " + cred;
            qDebug() << message;
            logMessage(message);
        }
    }
}

void Server_Interface::setUserOnline(QString &uname)
{
    connOpen();
    QSqlQuery qry;
    qry.exec("update Clients set isLoggedIn='true' where username='"+ uname +"'");
    connClose();
}

//void Server_Interface::privateChatClientCheck(QString &sender, QString &receiver)
//{
//    qDebug() << sender << receiver;
//    qDebug() << onlineUsers[0].first << onlineUsers[1].first;
//    for(int i=0; i < onlineUsers.length(); i++)
//    {
//        if(onlineUsers[i].first == receiver)
//        {
//            sendPrivateChatPass(sender, receiver);
//        }
//        else
//        {
//            sendPrivateChatFail(sender, receiver);
//        }
//    }
//}

void Server_Interface::sendPrivateChatPass(QString &sender, QString &receiver)
{
    QByteArray ba;
    ba.append("PRIV_MSG_PASS:" + sender.toUtf8() + ":" + receiver.toUtf8());
    qDebug() << "Failed to open chat from: " << sender << " to: " << receiver;
    for(int i = 0; i< onlineUsers.length(); ++i)
    {
        if(onlineUsers[i].first == sender)
        {
            onlineUsers[i].second->write(ba);
        }
    }
}

void Server_Interface::recievePrivateMessage(QString &sender, QString &receiver, QString&message)
{
    for(int i=0; i < onlineUsers.length(); i++)
    {
        if(onlineUsers[i].first == receiver)
        {
            QByteArray ba;
            ba.append("PRIV_MSG_RCV:" + sender.toUtf8() + ":" + message.toUtf8());
            qDebug() << "Sending message from: " << sender << " to " << receiver;
            onlineUsers[i].second->write(ba);
        }
        else
        {
            qDebug() << "Could not send " << message << " to: " << receiver << " from: " << sender;
        }
    }
}

//void Server_Interface::sendPrivateChatFail(QString &sender, QString &receiver)
//{
//    QByteArray ba;
//    ba.append("PRIV_MSG_FAIL:" + sender.toUtf8() + ":" + receiver.toUtf8());
//    qDebug() << "Failed to open chat from: " << sender << " to: " << receiver;
//    for(int i = 0; i< onlineUsers.length(); ++i)
//    {
//        if(onlineUsers[i].first == sender)
//        {
//            onlineUsers[i].second->write(ba);
//        }
//    }
//}


void Server_Interface::onClientDisconnected()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

//    for(int i = 0; i < onlineUsers.length(); i++)
//    {
//        if(onlineUsers[i].second == clientSocket)
//        {
//            qDebug() << "Removing user: "<< onlineUsers[i].first;
//            onlineUsers.removeOne(onlineUsers[i].first);
//            onlineUsers.removeOne(onlineUsers[i].second);
//        }
//    }

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            client_list.remove(clientSocket);
            clientSocket->abort();
            QString message = "Client disconnected.";
            logMessage(message);
        }
    }
}


void Server_Interface::sendActiveUsersList()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            connOpen();
            QStringList tempList;
            QSqlQuery qry;
            qry.prepare("select username from Clients where isLoggedIn='true'");
            if(qry.exec())
            {
                while(qry.next())
                {
                   tempList << qry.value(0).toString();
                }
            }
            qDebug() << "Temp List: " << tempList;
            QString data = tempList.join(":");
            QByteArray ba;
            ba.append("ACTIVE_USERS:" + data.toUtf8());
            clientSocket->write(ba);
            connClose();
        }
    }
}



void Server_Interface::on_pushButton_refreshActiveList_clicked()
{

}
