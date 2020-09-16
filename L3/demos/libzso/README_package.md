####package generation
#copy pre-compiled/ to current dirrectory
./pre-compiled folder should have lib file libz.so.${LIB_VERSION}, e.g. libz.so.1.2.7 and exe file xzlib ready.


#package generation options
-LIB_VERSION : set the lib version to be installed in the package
-PROD : 1(default) generate product version package  0 : generate cisco  version package

#Example:
#RPM package
run the following command, it will generate rpm product package  xzlib-1-2-11-2020-1-Linux.rpm for zlib1.2.11
```
make RPM LIB_VERSION=1.2.11 PROD=1
```
run the following command, it will generate rpm cisco package xzlib-1-2-11-2020-1-cisco-Linux.rpm for zlib1.2.11
```
make RPM LIB_VERSION=1.2.11 PROD=0
```

run the following command, it will generate rpm product package  xzlib-1-2-7-2020-1-Linux.rpm for zlib1.2.7
```
make RPM LIB_VERSION=1.2.7 PROD=1
```

#DEB package. change the target to DEB. other options are the same like RPM.
```
make DEB LIB_VERSION=1.2.7
make DEB LIB_VERSION=1.2.11
```

