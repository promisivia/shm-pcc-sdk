#include "connection/establish.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <libssh/libssh.h>
#include <string>
#include <thread>
#include <utility>

namespace shm_follower_detail {

std::string get_user_name() {
  const char *username = getenv("USER");
  if (username == nullptr) {
    std::cerr << "Get username error" << std::endl;
    exit(EXIT_FAILURE);
  }
  return std::string(username);
}

class SSHConnection {
public:
  SSHConnection(const char *h, const char *u, std::ostream &os) : os(os) {
    host = new char[strlen(h) + 1];
    user = new char[strlen(u) + 1];
    strcpy(host, h);
    strcpy(user, u);
    build_ssh_connection(host, user);
    connected = true;
  }

  SSHConnection(SSHConnection &&other) noexcept : os(other.os) {
    host = new char[strlen(other.host) + 1];
    user = new char[strlen(other.user) + 1];
    strcpy(host, other.host);
    strcpy(user, other.user);
    session = other.session;
    channel = other.channel;
    connected = other.connected;
    output_listener = std::move(other.output_listener);

    other.session = nullptr;
    other.channel = nullptr;
    other.connected = false;
  }

  SSHConnection(const SSHConnection &) = delete;
  SSHConnection &operator=(const SSHConnection &) = delete;

  ~SSHConnection() {
    if (output_listener.joinable()) {
      output_listener.join();
    }
    disconnect();
    delete[] host;
    delete[] user;
  }

  void build_ssh_connection(const char *host, const char *user);
  void listen() {
    if (output_listener.joinable()) {
      output_listener.join();
    }
    output_listener =
        std::thread(&SSHConnection::read_ssh_channel_output, this);
  }
  int ssh_exec(const char *command) {
    int rc = ssh_channel_request_exec(channel, command);
    if (rc != SSH_OK) {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
    }
    return rc;
  }
  void disconnect() {
    if (!connected) {
      return;
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    connected = false;
  }
  void read_ssh_channel_output();
  std::string get_error() { return std::string(ssh_get_error(session)); }

private:
  bool connected = false;
  ssh_session session = nullptr;
  ssh_channel channel = nullptr;
  char *host = nullptr;
  char *user = nullptr;
  std::thread output_listener;
  std::ostream &os;
};

} // namespace shm_follower_detail

struct FollowerManager::Impl {
  uint32_t followers_count = 0;
  std::vector<std::string> machines;
  std::vector<shm_follower_detail::SSHConnection> ssh_connections;
};

FollowerManager::FollowerManager(std::vector<std::string> machines)
    : impl_(std::make_unique<Impl>()) {
  impl_->followers_count = static_cast<uint32_t>(machines.size());
  impl_->machines = std::move(machines);
}

FollowerManager::~FollowerManager() = default;

FollowerManager::FollowerManager(FollowerManager &&) noexcept = default;

FollowerManager &FollowerManager::operator=(FollowerManager &&) noexcept = default;

void FollowerManager::start_followers(
    std::function<std::string(uint32_t)> build_command) {
  impl_->ssh_connections.reserve(impl_->followers_count);
  for (uint32_t i = 0; i < impl_->followers_count; i++) {
    impl_->ssh_connections.emplace_back(impl_->machines[i].c_str(),
                                       shm_follower_detail::get_user_name().c_str(),
                                       std::cout);

    auto &conn = impl_->ssh_connections.back();
    std::string exec_command = build_command(i + 1);
    std::cout << "Executing command on follower " << impl_->machines[i] << ": "
              << exec_command << std::endl;
    int rc = conn.ssh_exec(exec_command.c_str());

    if (rc != SSH_OK) {
      std::cerr << "Error executing command on follower " << impl_->machines[i]
                << ": " << conn.get_error() << std::endl;
      impl_->ssh_connections.clear();
      exit(1);
    }
    conn.listen();
  }
}

void FollowerManager::stop_followers() { impl_->ssh_connections.clear(); }

void shm_follower_detail::SSHConnection::build_ssh_connection(const char *host,
                                                              const char *user) {
  session = ssh_new();
  int verbosity = SSH_LOG_NOLOG;
  if (session == nullptr) {
    perror("ssh_new error");
    exit(EXIT_FAILURE);
  }
  ssh_options_set(session, SSH_OPTIONS_HOST, host);
  ssh_options_set(session, SSH_OPTIONS_USER, user);
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error connecting to %s: %s\n", host,
            ssh_get_error(session));
    ssh_free(session);
    exit(EXIT_FAILURE);
  }
  rc = ssh_userauth_publickey_auto(session, NULL, NULL);
  if (rc != SSH_AUTH_SUCCESS) {
    fprintf(stderr, "Public key authentication failed: %s\n",
            ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    exit(EXIT_FAILURE);
  }

  channel = ssh_channel_new(session);
  if (channel == nullptr) {
    perror("ssh_channel_new error");
    exit(EXIT_FAILURE);
  }
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK) {
    fprintf(stderr, "Error opening channel: %s\n", ssh_get_error(session));
    ssh_channel_free(channel);
    exit(EXIT_FAILURE);
  }
}

void shm_follower_detail::SSHConnection::read_ssh_channel_output() {
  char buffer[256];
  int nbytes;

  while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)) {
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    if (nbytes > 0) {
      // std::cout << "STDOUT: ";
      std::cout.write(buffer, nbytes);
    } else if (nbytes == SSH_ERROR) {
      std::cerr << "Error reading stdout: "
                << ssh_get_error(ssh_channel_get_session(channel)) << std::endl;
      break;
    }

    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1);
    if (nbytes > 0) {
      // std::cerr << "STDERR: ";
      std::cerr.write(buffer, nbytes);
    } else if (nbytes == SSH_ERROR) {
      std::cerr << "Error reading stderr: "
                << ssh_get_error(ssh_channel_get_session(channel)) << std::endl;
      break;
    }
  }
}
