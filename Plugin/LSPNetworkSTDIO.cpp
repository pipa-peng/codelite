#include "LSPNetworkSTDIO.h"
#include "JSON.h"
#include <sstream>
#include "file_logger.h"
#include "clcommandlineparser.h"

LSPNetworkSTDIO::LSPNetworkSTDIO() { Bind(wxEVT_END_PROCESS, &LSPNetworkSTDIO::OnProcessTerminated, this); }

LSPNetworkSTDIO::~LSPNetworkSTDIO()
{
    Unbind(wxEVT_END_PROCESS, &LSPNetworkSTDIO::OnProcessTerminated, this);
    Close();
}

void LSPNetworkSTDIO::Close()
{
    if(m_lspServer) {
        long pid = m_lspServer->GetPid();
        m_lspServer->Detach();
        wxKillError rc;
        ::wxKill(pid, wxSIGTERM, &rc, wxKILL_CHILDREN);
        m_lspServer = nullptr;
    }
    m_socket.reset(nullptr);
}

void LSPNetworkSTDIO::Open(const LSPStartupInfo& siInfo)
{
    m_startupInfo = siInfo;

    // Start the LSP server first
    Close();

    // Start the network server
    wxString connectionString;
    connectionString << "tcp://127.0.0.1:0";
    m_socket.reset(new clAsyncSocket(connectionString, kAsyncSocketBuffer | kAsyncSocketServer));
    m_socket->Bind(wxEVT_ASYNC_SOCKET_CONNECTED, &LSPNetworkSTDIO::OnSocketConnected, this);
    m_socket->Bind(wxEVT_ASYNC_SOCKET_SERVER_READY, &LSPNetworkSTDIO::OnSocketServerReady, this);
    m_socket->Bind(wxEVT_ASYNC_SOCKET_CONNECTION_LOST, &LSPNetworkSTDIO::OnSocketConnectionLost, this);
    m_socket->Bind(wxEVT_ASYNC_SOCKET_CONNECT_ERROR, &LSPNetworkSTDIO::OnSocketConnectionError, this);
    m_socket->Bind(wxEVT_ASYNC_SOCKET_ERROR, &LSPNetworkSTDIO::OnSocketError, this);
    m_socket->Bind(wxEVT_ASYNC_SOCKET_INPUT, &LSPNetworkSTDIO::OnSocketData, this);
    m_socket->Start();
}

void LSPNetworkSTDIO::Send(const std::string& data)
{
    if(m_socket) {
        clDEBUG() << "LSP socket:\n" << data;
        m_socket->Send(data);
    } else {
        clDEBUG() << "LSP socket: no socket !?";
    }
}

void LSPNetworkSTDIO::OnSocketConnected(clCommandEvent& event)
{
    clDEBUG() << "Connection established successfully.";
    
    // Build custom command that tells the helper to start the LSP server on its end
    JSON json(cJSON_Object);
    JSONItem item = json.toElement();

    clCommandLineParser parser(m_startupInfo.GetLspServerCommand());
    wxArrayString argv = parser.ToArray();
    if(argv.IsEmpty()) { return; }
    item.addProperty("command", argv.Item(0));
    argv.RemoveAt(0, 1);
    item.addProperty("args", argv);
    item.addProperty("wd", m_startupInfo.GetLspServerCommandWorkingDirectory().IsEmpty()
                               ? ::wxGetCwd()
                               : m_startupInfo.GetLspServerCommandWorkingDirectory());
    item.addProperty("method", wxString("execute"));

    char* str = item.FormatRawString(true);
    std::string jsonStr = str;
    free(str);

    std::stringstream ss;
    ss << "Content-Length: " << jsonStr.length() << "\r\n\r\n";
    ss << jsonStr;

    // Send the data
    clDEBUG() << "Sending request\n" << ss.str();
    Send(ss.str());
    
    // Now that everything is up, send the "initialize" request
    clCommandEvent evtReady(wxEVT_LSP_NET_CONNECTED);
    AddPendingEvent(evtReady);
}

void LSPNetworkSTDIO::OnSocketConnectionLost(clCommandEvent& event)
{
    clCommandEvent evt(wxEVT_LSP_NET_ERROR);
    evt.SetString(event.GetString());
    AddPendingEvent(evt);
}

void LSPNetworkSTDIO::OnSocketConnectionError(clCommandEvent& event)
{
    clCommandEvent evt(wxEVT_LSP_NET_ERROR);
    evt.SetString(event.GetString());
    AddPendingEvent(evt);
}

void LSPNetworkSTDIO::OnSocketError(clCommandEvent& event)
{
    clCommandEvent evt(wxEVT_LSP_NET_ERROR);
    evt.SetString(event.GetString());
    AddPendingEvent(evt);
}

void LSPNetworkSTDIO::OnSocketData(clCommandEvent& event)
{
    const wxString& dataRead = event.GetString();
    clCommandEvent evt(wxEVT_LSP_NET_DATA_READY);
    evt.SetString(dataRead);
    AddPendingEvent(evt);
}

bool LSPNetworkSTDIO::IsConnected() const { return m_socket != nullptr; }

void LSPNetworkSTDIO::OnProcessTerminated(wxProcessEvent& event)
{
    wxDELETE(m_lspServer);
    clDEBUG() << "LSPNetworkSTDIO: helper program terminated:" << m_startupInfo.GetLspServerCommand();
    clCommandEvent evt(wxEVT_LSP_NET_ERROR);
    AddPendingEvent(evt);
}

void LSPNetworkSTDIO::OnSocketServerReady(clCommandEvent& event)
{
    // Now that the socket server is ready and we know which port we bound, start the helper script
    int port = event.GetInt();
    wxString helperScript;
    helperScript << m_startupInfo.GetHelperCommand() << " " << port;
    clDEBUG() << "Starting helper script:" << helperScript;

    // First, start the helper script
    m_lspServer = new wxProcess(this);
    clDEBUG() << " >>> Waiting on 127.0.0.1:" << port;

    size_t flags = wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER | wxEXEC_HIDE_CONSOLE;
    if(m_startupInfo.GetFlags() & LSPStartupInfo::kShowConsole) { flags &= ~wxEXEC_HIDE_CONSOLE; }

    if(::wxExecute(helperScript, flags, m_lspServer) <= 0) {
        clCommandEvent evt(wxEVT_LSP_NET_ERROR);
        evt.SetString(wxString() << "Failed to execute: " << m_startupInfo.GetLspServerCommand());
        AddPendingEvent(evt);
        return;
    }
}
