#ifndef SERVER_INTERFACE_H
#define SERVER_INTERFACE_H

#include <QWidget>
#include <QObject>
#include <QDebug>
#include <QtSql>
#include <QDateTime>
#include <QList>
#include <QTcpSocket>
#include <QMessageBox>
#include <QTcpServer>
#include <QStringListModel>
#include <QPair>

class Server_Client;

QT_BEGIN_NAMESPACE
namespace Ui { class Server_Interface; }
QT_END_NAMESPACE

class Server_Interface : public QWidget
{
    Q_OBJECT

public:
    Server_Interface(QWidget *parent = nullptr);
    ~Server_Interface();

    QSqlDatabase sqlDb;

    bool connOpen()
    {
        sqlDb = QSqlDatabase::addDatabase("QMYSQL");
        sqlDb.setHostName("127.0.0.1");
        sqlDb.setPort(3306);
        sqlDb.setDatabaseName("TJ_Client_Server_DB");
        sqlDb.setUserName("admin");
        sqlDb.setPassword("F5UoJRaSfPdC");

        if(!sqlDb.open())
        {
            QString message = "Cannot connect to database";
            logMessage(message);
            qDebug() << message;
        }
        else
        {
            QString message = "Connected to database!";
            emit logMessage(message);
            qDebug() << message;
        }
    }

    void connClose()
    {
        QString message = "Closing the connection to Database";
        logMessage(message);
        qDebug() << message;
        sqlDb.close();
    }

    void ServerOnOrOff();
    void discardAllClients();

public slots:
    void logMessage(const QString &msg);
    void onReadyRead();
    void onClientDisconnected();
    void checkLoginCreds(QString &userName, QString &password);
    void loginSuccess(QString &username, QString &password);
    void loginFail(QString &creds);
    void loginDuplicate(QString &creds);
    void setUserOnline(QString &uname);
    void sendActiveUsersList();
    void privateChatClientCheck(QString &sender, QString &receiver);
    void recievePrivateMessage(QString &sender, QString &receiver, QString &text);
    void sendPrivateChatFail(QString &sender, QString &receiver);
    void sendPrivateChatPass(QString &sender, QString &receiver);
//    void sendMessage(QTcpSocket* client, QByteArray &ba);

private slots:
    void newConnectionMade();
//    void sendLoginCheck(QString &uname,bool &status);
    void on_pushButton_refreshActiveList_clicked();


signals:

private:
    Ui::Server_Interface *ui;
    QTcpServer *server;
    QSet<QTcpSocket*> client_list;
    QTcpSocket *serverSocket;
    QVector<QPair<QString,QTcpSocket*>> onlineUsers;
    QStringList tempList;
};
#endif // SERVER_INTERFACE_H
