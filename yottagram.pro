include(vendor/vendor.pri)

TARGET = yottagram

QT += dbus multimedia

CONFIG +=  c++11 c++14 link_pkgconfig sailfishapp iostream

QMAKE_CXXFLAGS += -std=c++14 -O0

PKGCONFIG += zlib openssl nemonotifications-qt5 connman-qt5 vorbisfile

SOURCES += src/core.cpp \
    src/components/audiorecorder.cpp \
    src/components/autodownloadsettings.cpp \
    src/components/basicgroupfullinfo.cpp \
    src/components/scopenotificationsettings.cpp \
    src/components/supergroupfullinfo.cpp \
    src/components/thumbnail.cpp \
    src/components/userfullinfo.cpp \
    src/files/animation.cpp \
    src/files/audio.cpp \
    src/files/contentfile.cpp \
    src/files/document.cpp \
    src/files/file.cpp \
    src/files/files.cpp \
    src/files/photo.cpp \
    src/files/sticker.cpp \
    src/files/video.cpp \
    src/files/videonote.cpp \
    src/files/voicenote.cpp \
    src/message.cpp \
    src/notifications.cpp \
    src/poll.cpp \
    src/stickerset.cpp \
    src/stickersets.cpp \
    src/user.cpp \
    src/users.cpp \
    src/webpage.cpp \
    src/yottagram.cpp \
    src/authorization.cpp \
    src/core/telegramreceiver.cpp \
    src/core/telegrammanager.cpp \
    src/chatlist.cpp \
    src/chat.cpp

DISTFILES += qml/yottagram.qml \
    com.verdanditeam.yottagram.service \
    qml/components/AutoDownloadSettings.qml \
    qml/components/ChatNotifications.qml \
    qml/components/PinnedMessage.qml \
    qml/components/ScopeNotificationSettings.qml \
    qml/components/StickerPicker.qml \
    qml/components/SubpageElement.qml \
    qml/components/SwitchButton.qml \
    qml/components/chatInfo/BasicGroupInfo.qml \
    qml/components/chatInfo/SupergroupInfo.qml \
    qml/components/chatInfo/UserInfo.qml \
    qml/components/functions/muteFormat.js \
    qml/components/messageContent/AnimationContent.qml \
    qml/components/messageContent/AudioContent.qml \
    qml/components/Avatar.qml \
    qml/components/messageContent/DocumentContent.qml \
    qml/components/HighlightLabelIconButton.qml \
    qml/components/messageContent/ImageContent.qml \
    qml/components/LabelWithMenu.qml \
    qml/components/MessageBubble.qml \
    qml/components/MessageContextMenu.qml \
    qml/components/messageContent/Poll.qml \
    qml/components/messageContent/StickerContent.qml \
    qml/components/TextSwitchWithMenu.qml \
    qml/components/messageContent/VideoContent.qml \
    qml/components/messageContent/VideoNoteContent.qml \
    qml/components/messageContent/VoiceNoteContent.qml \
    qml/cover/CoverPage.qml \
    qml/dialogs/PollDialog.qml \
    qml/dialogs/SetTtlDialog.qml \
    qml/pages/About.qml \
    qml/pages/AuthorizationCode.qml \
    qml/pages/AuthorizationNumber.qml \
    qml/pages/AuthorizationPassword.qml \
    qml/pages/Chat.qml \
    qml/pages/ChatList.qml \
    qml/pages/Loading.qml \
    qml/pages/Settings.qml \
    rpm/yottagram.spec \
    translations/*.ts \
    yottagram.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n

# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
 TRANSLATIONS += translations/yottagram-es.ts

HEADERS += \
    src/components/audiorecorder.h \
    src/components/autodownloadsettings.h \
    src/components/basicgroupfullinfo.h \
    src/components/scopenotificationsettings.h \
    src/components/supergroupfullinfo.h \
    src/components/thumbnail.h \
    src/components/userfullinfo.h \
    src/core.h \
    src/files/animation.h \
    src/files/audio.h \
    src/files/contentfile.h \
    src/files/document.h \
    src/files/file.h \
    src/files/files.h \
    src/files/photo.h \
    src/files/sticker.h \
    src/files/video.h \
    src/files/videonote.h \
    src/files/voicenote.h \
    src/message.h \
    src/notifications.h \
    src/overloaded.h \
    src/authorization.h \
    src/core/telegramreceiver.h \
    src/core/telegrammanager.h \
    src/chatlist.h \
    src/chat.h \
    src/poll.h \
    src/stickerset.h \
    src/stickersets.h \
    src/user.h \
    src/users.h \
    src/webpage.h

LIBS = -lssl -pthread /usr/lib/libtdclient.so.1.6.0 /usr/lib/libtdcore.a /usr/lib/libtdutils.a

RESOURCES += \
    lottie.qrc \
    qml/resources/icons.qrc

dbus.files = com.verdanditeam.yottagram.service
dbus.path = /usr/share/dbus-1/services/

notificationcategories.files=x-verdanditeam.yottagram.im.conf
notificationcategories.path=/usr/share/lipstick/notificationcategories

INSTALLS += dbus notificationcategories
