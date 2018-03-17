#include "TcpClient.h"
#include "TcpSession.h"
#include <logger.h>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>

using boost::asio::ip::tcp;
using namespace boost::asio;

TcpClient::TcpClient(uint32_t entry_type, DecoderFn decoder) {
  this->m_EntryType = entry_type;
  this->m_decoder = decoder;
}

TcpClient::~TcpClient(void) {
  DEBUG_LOG(logger)("TcpClient destroy");
}

bool TcpClient::Connect(const char *szIP, const char *szPort, const IoServicePtr& io) {
  m_pSocket.reset(new tcp::socket(*io));

  tcp::resolver resolver(*io);
  tcp::resolver::query query(tcp::v4(), szIP, szPort);
  tcp::resolver::iterator iterator = resolver.resolve(query);
  ip::tcp::endpoint ep = *iterator;

  boost::system::error_code e;
  if (m_pSocket && m_pSocket->is_open()) {
    m_pSocket->close(e);
    m_pSocket->connect(ep, e);
  } else {
    m_pSocket->connect(ep, e);
  }

  static tcp::no_delay option(true);
  m_pSocket->set_option(option);
  m_pSession = boost::make_shared<TcpSession>(io, this->m_pSocket);
  m_pSession->SetEntryType(this->m_EntryType);
  m_pSession->SetDecoder(this->m_decoder);
  std::string client_account(szIP);
  client_account += ":";
  client_account += "szPort";
  m_pSession->SetAccount(client_account);

  if (e) {
    DEBUG_LOG(logger)("connect error %s:%s, %s", szIP, szPort, e.message().c_str());
    CloseSocket();
    return false;
  }
  if (io->stopped()) {
    io->reset();
    return false;
  }

  TcpSessionManager::Instance().AddTcpSession(m_pSession);
  m_pSession->Start();
  return true;
}

void TcpClient::CloseSocket() {
  if (this->m_pSession) {
    TcpSessionManager::Instance().RemoveTcpSession(this->m_pSession.get());
    this->m_pSession->Close();
    this->m_pSession.reset();
  }
  this->m_pSocket.reset();
}

bool TcpClient::ASyncWrite(char *pData, size_t nLen) {
  if (!m_pSocket || !m_pSocket->is_open()) {
    DEBUG_LOG(logger)("ASyncWrite: session is closed , failed to write!");
    return false;
  }
  TcpSessionPtr session = this->m_pSession;
  if (!session) {
    return false;
  }

  session->ASyncWrite(pData, nLen);
  return true;
}

