# Web Platform

Web platform is secure NodeJS implementation of Web APIs.

## Short FAQ

### Why?

For personal in-depth learning of current Web APIs. Learning by doing is the best.

### Doesn't jsdom package do the same?

No, executing random JavaScript code in jsdom is insecure and specially designed code can obtain access to native NodeJS modules and take over your computer. According to jsdom, it is risk to execute code from the internet.

Web browser execute code from the internet all the time. Their implementation is sandboxed, which makes JavaScript code from random website safer than executing an operating system executable.

This library similar to a browser, allow executing of sandboxed code (if modified/extended with care). Unlike the browsers, it makes it possible to modify/extend the behavior easier than modifying the C++/Rust core and re-compile an open source browser.
