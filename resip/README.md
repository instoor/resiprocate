#  Steps to fix compilation and linker issues for iOS

1. move source files from resiprocate/rutil/dns to resiprocate/contrib/ares
2. change Base sdk from macOS to iOS latest
3. fix 'openssl/evp.h' file not found by adding headers search path for OpenSSL
- Download [https://github.com/x2on/OpenSSL-for-iPhone] zip archive and copy to resiprocate folder.
- ./build-libssl.sh --version=1.1.0i --archs="x86_64 i386 arm64 armv7s armv7"
4. Project settings -> resiprocate target settings -> Build phases -> Link binary with libraries
Add libssl.a and libcrypto.a from resiprocate/openssl/lib
5. Update User Header search paths to:
../openssl/include
6. `field has incomplete type 'EVP_MD_CTX' (aka 'evp_md_ctx_st')` error.
It seems it is due to usage of latest OpenSSL 1.1.x , need to use 1.0.2 version to fix that.
./build-libssl.sh --version=1.0.2p --archs="x86_64 i386 arm64 armv7s armv7"
[https://github.com/randombit/botan/issues/753]

