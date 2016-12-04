#ifndef NETWORK_ACCESS_MANAGER_HPP__
#define NETWORK_ACCESS_MANAGER_HPP__

#include <QNetworkAccessManager>
#include <QList>
#include <QSslError>
#include <QNetworkReply>
#include <QString>

#include "MessageBox.hpp"

class QNetworkRequest;
class QIODevice;
class QWidget;

// sub-class QNAM to keep a list of accepted SSL errors and allow
// them in future replies
class NetworkAccessManager
  : public QNetworkAccessManager
{
public:
  NetworkAccessManager (QWidget * parent = nullptr)
    : QNetworkAccessManager (parent)
  {
    // handle SSL errors that have not been cached as allowed
    // exceptions and offer them to the user to add to the ignored
    // exception cache
    connect (this, &QNetworkAccessManager::sslErrors, [this, &parent] (QNetworkReply * reply, QList<QSslError> const& errors) {
        QString message;
        for (auto const& error: errors)
          {
            message += '\n' + reply->request ().url ().toDisplayString () + ": "
              + error.errorString ();
          }
        QString certs;
        for (auto const& cert : reply->sslConfiguration ().peerCertificateChain ())
          {
            certs += cert.toText () + '\n';
          }
        if (MessageBox::Ignore == MessageBox::query_message (parent, tr ("Network SSL Errors"), message, certs, MessageBox::Abort | MessageBox::Ignore))
          {
            // accumulate SSL error exceptions that have been allowed
            allowed_ssl_errors_.append (errors);
            reply->ignoreSslErrors (errors);
          }
      });
  }

protected:
  QNetworkReply * createRequest (Operation operation, QNetworkRequest const& request, QIODevice * outgoing_data = nullptr) override
  {
    auto reply = QNetworkAccessManager::createRequest (operation, request, outgoing_data);
    // errors are usually certificate specific so passing all cached
    // exceptions here is ok
    reply->ignoreSslErrors (allowed_ssl_errors_);
    return reply;
  }

private:
  QList<QSslError> allowed_ssl_errors_;
};

#endif
