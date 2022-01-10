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

    else if (signal == "REGISTER")
        command = 4;
    else if (signal == "GET_PROF_INFO")
        command = 5;
    else if (signal == "NEW_PROF_INFO")
        command = 6;
    else if (signal == "GET_CONTACTS")
        command=7;

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
    case 4:
    {
        qDebug() << dataRead;
        newRegistration(dataRead);
        break;
    }
    case 5:
    {
        sendProfileInfo();
        break;
    }
    case 6:
    {
        newProfInfo(dataRead);
        break;
    }
    case 7:
    {
        sendContactsList();
        break;
    }
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
    qry.exec("update Clients set isLoggedIn='True' where username='"+ uname +"'");
    connClose();
}

void Server_Interface::newRegistration(QStringList &list)
{
    QStringList regList;
    list.removeAt(0);
    regList = list;
    qDebug() << regList << regList[0] << regList[1];

    QString newFirstName = regList[0];
    QString newLastName = regList[1];
    QString newUserName = regList[2];
    QString newPassword = regList[3];
    QString newAge = regList[4];
    QString newCity = regList[5];
    QString newSex = regList[6];

    int ageFromString = newAge.toInt();

    qDebug() << newFirstName << newLastName << newUserName << newPassword << ageFromString << newCity << newSex;

    connOpen();
    QSqlQuery qry;
    qry.prepare("insert into Clients (firstName,lastName,username,pword,age,city,sex) values (:firstName,:lastName,:userName,:pword,:age,:city,:sex)");
    qry.bindValue(":firstName", newFirstName);
    qry.bindValue(":lastName", newLastName);
    qry.bindValue(":userName", newUserName);
    qry.bindValue(":pword", newPassword);
    qry.bindValue(":age", ageFromString);
    qry.bindValue(":city", newCity);
    qry.bindValue(":sex", newSex);
    qry.exec();
    connClose();
}



void Server_Interface::sendPrivateChatMessage(QString &sender, QString &receiver)
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

void Server_Interface::onClientDisconnected()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());
    for(int i=0; i < onlineUsers.length(); i++)
    {
        if(onlineUsers[i].second == clientSocket)
        {
            connOpen();
            QSqlQuery qry;
            QString username=onlineUsers[i].first;
            QString status = "False";
            qry.prepare("update Clients set isLoggedIn=:false where username=:username");
            qry.bindValue("::false", status);
            qry.bindValue(":username", username);
            qry.exec();
            connClose();
        }
    }

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            clientSocket->close();
            client_list.remove(clientSocket);
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
            qry.prepare("select username from Clients where isLoggedIn='True'");
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

void Server_Interface::refreshList()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            connOpen();
            QStringList refreshList;
            QSqlQuery qry;
            qry.prepare("select username from Clients where isLoggedIn='True'");
            if(qry.exec())
            {
                while(qry.next())
                {
                   refreshList << qry.value(0).toString();
                }
            }
            qDebug() << "Refreshed List: " << refreshList;
            QString data = refreshList.join(":");
            QByteArray ba;
            ba.append("REFRESH_LIST:" + data.toUtf8());
            clientSocket->write(ba);
            connClose();
        }
    }
}

void Server_Interface::sendContactsList()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

    for(auto i : client_list)
    {
        if (i == clientSocket)
        {
            connOpen();
            QStringList contactsList;
            QSqlQuery qry;
            qry.prepare("select firstName from Clients");
            if(qry.exec())
            {
                while(qry.next())
                {
                   contactsList << qry.value(0).toString();
                }
            }
            qDebug() << "Contacts List: " << contactsList;
            QString data = contactsList.join(":");
            QByteArray ba;
            ba.append("CONTACT_LIST:" + data.toUtf8());
            clientSocket->write(ba);
            connClose();
        }
    }
}


void Server_Interface::sendProfileInfo()
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

    for(int i=0; i<onlineUsers.length(); i++)
    {
        if(onlineUsers[i].second == clientSocket)
        {
            QString name = onlineUsers[i].first;
            qDebug() << name;

            connOpen();

            QSqlQuery qry;
            qry.prepare("select * from Clients where username=:uname");
            qry.bindValue(":uname", name);
            if(qry.exec())
            {
                QVector<QStringList> list;
                while(qry.next())
                {
                    QSqlRecord record = qry.record();
                    QStringList tempList;
                    for(int j = 0; j < record.count(); j++)
                    {
                        tempList << record.value(j).toString();
                    }
                    tempList.removeAt(0);
                    qDebug() << "Temp List: " << tempList;
                    list.append(tempList);
                }
                qDebug() << "Vector List: " << list;
                foreach(const QStringList &var,list)
                {
                    qDebug() << var;
                    QString data = var.join(":");
                    QByteArray ba;
                    ba.append("PROF_INFO:" + data.toUtf8());
                    onlineUsers[i].second->write(ba);

                }
                connClose();
            }
        }
    }
}

void Server_Interface::newProfInfo(QStringList &list)
{
    QTcpSocket *clientSocket = static_cast<QTcpSocket *>(QObject::sender());

    for(int i=0; i<onlineUsers.length(); i++)
    {
        if(onlineUsers[i].second == clientSocket)
        {
            QString name = onlineUsers[i].first;
            qDebug() << name;
            QStringList profList;
            list.removeAt(0);
            profList = list;
            qDebug() << profList << profList[0] << profList[1];

            QString newFirstName = profList[0];
            QString newLastName = profList[1];
            QString newUserName = profList[2];
            QString newPassword = profList[3];
            QString newAge = profList[4];
            QString newCity = profList[5];
            QString newSex = profList[6];

            int ageFromString = newAge.toInt();

            qDebug() << newFirstName << newLastName << newUserName << newPassword << ageFromString << newCity << newSex;

            connOpen();
            QSqlQuery qry;
            qry.prepare("update Clients set firstName=:firstName,lastName=:lastName,username=:userName,pword=:pword,age=:age,city=:city,sex=:sex where username=:name");
            qry.bindValue(":firstName", newFirstName);
            qry.bindValue(":lastName", newLastName);
            qry.bindValue(":userName", newUserName);
            qry.bindValue(":pword", newPassword);
            qry.bindValue(":age", ageFromString);
            qry.bindValue(":city", newCity);
            qry.bindValue(":sex", newSex);
            qry.bindValue(":name", name);
            qry.exec();
            connClose();
        }
    }
}

