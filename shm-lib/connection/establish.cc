#include "connection/establish.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <libssh/libssh.h>
#include <string>

/* const int machine_N = 2; */
// int SimThreadInfo::worker_machine_id;

// const char *machines[NUM_CLIENTS] =
// #ifndef TEST_SINGLE_MACHINE
//     {"master", "master"};
// #else
//     {"master"};
// #endif

// ssh_session slave_sessions[NUM_CLIENTS];
// ssh_channel slave_channels[NUM_CLIENTS];
// int slave_pid[NUM_CLIENTS];

void FollowerManager::start_followers(
    std::function<std::string(uint32_t)> build_command) {

  // ssh_connections.reserve(followers_count);
  ssh_connections.reserve(followers_count);
  for (ulong i = 0; i < followers_count; i++) {
    ssh_connections.emplace_back(machines[i].c_str(), get_user_name().c_str(),
                                 std::cout);

    auto &conn = ssh_connections.back();
    std::string exec_command = build_command(i + 1);
    std::cout << "Executing command on follower " << machines[i] << ": "
              << exec_command << std::endl;
    int rc = conn.ssh_exec(exec_command.c_str());

    if (rc != SSH_OK) {
      std::cerr << "Error executing command on follower " << machines[i] << ": "
                << conn.get_error() << std::endl;
      ssh_connections.clear();
      exit(1);
    }
    conn.listen();
  }
}

void FollowerManager::stop_followers() { ssh_connections.clear(); }

std::string FollowerManager::get_user_name() {
  const char *username = getenv("USER");
  if (username == nullptr) {
    std::cerr << "Get username error" << std::endl;
    exit(EXIT_FAILURE);
  }
  return std::string(username);
}

void SSHConnection::build_ssh_connection(const char *host, const char *user) {
  session = ssh_new();
  int verbosity = SSH_LOG_WARNING;
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

void SSHConnection::read_ssh_channel_output() {
  char buffer[256];
  int nbytes;

  while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)) {
    // 读取 stdout
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    if (nbytes > 0) {
      std::cout << "STDOUT: ";
      std::cout.write(buffer, nbytes);
    } else if (nbytes == SSH_ERROR) {
      std::cerr << "Error reading stdout: "
                << ssh_get_error(ssh_channel_get_session(channel)) << std::endl;
      break;
    }

    // 读取 stderr
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1);
    if (nbytes > 0) {
      std::cerr << "STDERR: ";
      std::cerr.write(buffer, nbytes);
    } else if (nbytes == SSH_ERROR) {
      std::cerr << "Error reading stderr: "
                << ssh_get_error(ssh_channel_get_session(channel)) << std::endl;
      break;
    }
  }
}
