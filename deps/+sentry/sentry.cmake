if(UNIX AND NOT APPLE)
    set(_sentry_backend breakpad)
else()
    set(_sentry_backend crashpad)
endif ()

add_cmake_project(sentry
    URL https://github.com/getsentry/sentry-native/releases/download/0.10.1/sentry-native.zip
    URL_HASH SHA256=ab49c03879d83462cfca95abeaf0cb08fb2b54f6c2bbc1962dcded272b009272
    CMAKE_ARGS
        -DSENTRY_BACKEND=${_sentry_backend}
)
