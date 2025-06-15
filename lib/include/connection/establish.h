#pragma once

#include <cstring>
#include <functional>
#include <libssh/libssh.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>

// #define MACHINE_N (2)
// // #define TEST_SINGLE_MACHINE

#ifndef TEST_SINGLE_MACHINE
#define NUM_CLIENTS 2
#else
#define NUM_CLIENTS 1
#endif

// extern int SimThreadInfo::worker_machine_id;
// extern const char *machines[];
// extern ssh_session slave_sessions[];
// extern ssh_channel slave_channels[];
// extern int slave_pid[];

// class SlaveManager {
// public:
//   SlaveManager() {}
//   ~SlaveManager() {}
//   void *monitor_shutdown(void *arg);
// };

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

  SSHConnection(SSHConnection &&other) : os(other.os) {
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
    if(output_listener.joinable()) {
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
  ssh_session session;
  ssh_channel channel;
  char *host;
  char *user;
  std::thread output_listener;
  std::ostream &os;
};

class FollowerManager {
public:
  FollowerManager(std::vector<std::string> machines)
      : followers_count(machines.size()), machines(machines) {}
  ~FollowerManager() {}
  void start_followers(std::function<std::string(uint32_t)> build_command);
  void stop_followers();

  static std::vector<std::string>
  split_machines(const std::string &machines_str) {
    std::string machines_str_copy = machines_str;
    std::vector<std::string> machines;
    size_t pos = 0;
    while ((pos = machines_str_copy.find(',')) != std::string::npos) {
      machines.push_back(machines_str_copy.substr(0, pos));
      machines_str_copy.erase(0, pos + 1);
    }
    if (!machines_str_copy.empty()) {
      machines.push_back(machines_str_copy);
    }
    return machines;
  }

private:
  std::string get_user_name();

  uint32_t followers_count = 0;
  std::vector<std::string> machines;
  std::vector<SSHConnection> ssh_connections;
};
