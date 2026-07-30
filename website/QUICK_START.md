# 快速开始指南

## 🚀 一键启动文档网站

### 方法 1: 使用启动脚本（推荐）

```bash
cd website
./serve.sh
```

脚本会自动完成所有步骤，然后显示访问地址。

### 方法 2: 手动启动

```bash
cd website
bundle install
bundle exec jekyll serve --host 0.0.0.0
```

## 📍 访问地址

启动成功后，在浏览器中访问：

- **本地访问**: http://localhost:4000/shm-pcc-sdk/
- **网络访问**: http://YOUR_IP:4000/shm-pcc-sdk/

## 🛑 停止服务器

按 `Ctrl+C` 停止服务器

## 🔨 构建静态网站

如果只需要构建网站而不启动服务器：

```bash
cd website
./build.sh
```

构建的文件会在 `_site/` 目录中。

## ❓ 常见问题

### Q: 端口被占用怎么办？

A: 脚本会自动尝试使用 4001 端口。也可以手动指定：

```bash
bundle exec jekyll serve --port 4001
```

### Q: 无法访问网站？

A: 检查以下几点：
1. 确保服务器正在运行（查看终端输出）
2. 检查防火墙设置
3. 尝试使用 `localhost` 而不是 IP 地址
4. 确认端口号正确

### Q: 依赖安装失败？

A: 确保已安装 Ruby 和开发工具：

```bash
sudo apt-get install ruby-full build-essential zlib1g-dev
gem install bundler --user-install
```

### Q: 修改文档后没有更新？

A: Jekyll 会自动检测更改并重新加载。如果没有，重启服务器。

## 📚 更多帮助

查看 `README.md` 了解更多详细信息。



