Long: socks5-hostname
Arg: <host[:port]>
Help: SOCKS5 proxy, pass host name to proxy
Added: 7.18.0
---
Use the specified SOCKS5 proxy (and let the proxy resolve the host name). If
the port number is not specified, it is assumed at port 1080.

This option overrides any previous use of --proxy, as they are mutually
exclusive.

Since 7.21.7, this option is superfluous since you can specify a socks5
hostname proxy with --proxy using a socks5h:// protocol prefix.

If this option is used several times, the last one will be used.
