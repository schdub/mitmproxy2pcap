TEMPLATE = app
CONFIG  += console
CONFIG  += c++11
CONFIG  -= app_bundle
CONFIG  -= qt
LIBS    += -lpcap
SOURCES += mflow.cpp
win32 {
RC_FILE += winres.rc
}
