#pragma once

class LockBase {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual void r_lock() { lock(); }
  virtual void r_unlock() { unlock(); }
  virtual ~LockBase() = default;
};