/*

This file is part of Yottagram.
Copyright 2020, Michał Szczepaniak <m.szczepaniak.000@gmail.com>

Yottagram is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Yottagram is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Yottagram. If not, see <http://www.gnu.org/licenses/>.

*/

#include "chat.h"

#include <QDateTime>
#include <QFileInfo>
#include <QDesktopServices>
#include <QQmlEngine>
#include "overloaded.h"

Chat::Chat(td_api::chat* chat, shared_ptr<Files> files) : _chat(chat), _files(files), _secretChat(nullptr), _notificationSettings(nullptr), _scopeNotificationSettings(nullptr)
{
    _basicGroupFullInfo = new BasicGroupFullInfo();
    _supergroupFullInfo = new SupergroupFullInfo();
    _notificationSettings = move(chat->notification_settings_);
    setLastReadInboxMessageId(chat->last_read_inbox_message_id_);
    setLastReadOutboxMessageId(chat->last_read_outbox_message_id_);
    setUnreadCount(_chat->unread_count_);

    if (this->hasPhoto()) {
        setChatPhoto(std::move(chat->photo_));
    }
}

Chat::~Chat()
{
    delete _secretChat;
    delete _chat;
}

qint64 Chat::getId() const
{
    return _chat->id_;
}

qint32 Chat::getIdFromType() const
{
    switch (_chat->type_->get_id()) {
    case td_api::chatTypeBasicGroup::ID:
        return static_cast<td_api::chatTypeBasicGroup*>(_chat->type_.get())->basic_group_id_;
    case td_api::chatTypePrivate::ID:
        return static_cast<td_api::chatTypePrivate*>(_chat->type_.get())->user_id_;
    case td_api::chatTypeSecret::ID:
        return static_cast<td_api::chatTypeSecret*>(_chat->type_.get())->user_id_;
    case td_api::chatTypeSupergroup::ID:
        return static_cast<td_api::chatTypeSupergroup*>(_chat->type_.get())->supergroup_id_;
    default:
        return -1;
    }
}

qint32 Chat::getSecretChatId() const
{
    return static_cast<td_api::chatTypeSecret*>(_chat->type_.get())->secret_chat_id_;
}

QString Chat::getTitle() const
{
    return QString::fromStdString(_chat->title_);
}

UserFullInfo* Chat::getUserFullInfo() const
{
    if (_chat->type_->get_id() == td_api::chatTypePrivate::ID) {
        auto fullInfo = _users->getUser(static_cast<td_api::chatTypePrivate*>(_chat->type_.get())->user_id_)->getUserFullInfo();
        QQmlEngine::setObjectOwnership(fullInfo, QQmlEngine::CppOwnership);
        return fullInfo;
    }

    return nullptr;
}

BasicGroupFullInfo *Chat::getBasicGroupFullInfo() const
{
    return _basicGroupFullInfo;
}

SupergroupFullInfo *Chat::getSupergroupFullInfo() const
{
    return _supergroupFullInfo;
}

qint64 Chat::getOrder() const
{
    return _chat->order_;
}

void Chat::setOrder(int64_t order)
{
    _chat->order_ = order;
}

QString Chat::getChatType() const
{
    switch (_chat->type_->get_id()) {
    case td_api::chatTypeBasicGroup::ID:
        return "group";
    case td_api::chatTypePrivate::ID:
        return "private";
    case td_api::chatTypeSecret::ID:
        return "secret";
    case td_api::chatTypeSupergroup::ID:
        if (static_cast<td_api::chatTypeSupergroup*>(_chat->type_.get())->is_channel_) return "channel";
        return "supergroup";
    default:
        return "";
    }
}

int Chat::getChatList() const
{
    qint32 id;
    if (_chat->chat_list_ == nullptr) id = td_api::chatListMain::ID;
    else id = _chat->chat_list_->get_id();

    switch (id) {
    case td_api::chatListArchive::ID:
        return ChatList::Archive;
    case td_api::chatListMain::ID:
    default:
        return ChatList::Main;
    }
}

qint32 Chat::getUnreadCount() const
{
    return _unreadCount;
}

void Chat::setUnreadCount(qint32 unreadCount)
{
    _unreadCount = unreadCount;

    emit unreadCountChanged(this->getId(), unreadCount);
}

void Chat::setLastReadInboxMessageId(qint64 messageId)
{
    _lastReadInboxMessageId = messageId;
    emit lastReadInboxMessageIdChanged(messageId);

}

void Chat::setLastReadOutboxMessageId(qint64 messageId)
{
    _lastReadOutboxMessageId = messageId;
    emit lastReadOutboxMessageIdChanged(messageId);
}

void Chat::setChatPhoto(td_api::object_ptr<td_api::chatPhoto> chatPhoto)
{
    if (chatPhoto == nullptr) {
        _smallPhotoId = 0;
        _bigPhotoId = 0;
    } else {
        _smallPhotoId = chatPhoto->small_->id_;
        _files->appendFile(std::move(chatPhoto->small_), "avatar");
        _bigPhotoId = chatPhoto->big_->id_;
        _files->appendFile(std::move(chatPhoto->big_), "");
    }
    _chat->photo_ = std::move(chatPhoto);
    emit chatPhotoChanged(getId());
}

bool Chat::isSelf() const
{
    if (_chat->type_->get_id() != td_api::chatTypePrivate::ID) return false;

    qint32 userId = getIdFromType();
    return userId == _manager->getMyId();
}

void Chat::setIsOpen(bool isOpen)
{
    _isOpen = isOpen;
}

bool Chat::isOpen() const
{
    return _isOpen;
}

void Chat::setSecretChat(td_api::secretChat *secretChat)
{
    _secretChat = secretChat;
}

QString Chat::getSecretChatState() const
{
    if (_secretChat == nullptr) return "";

    switch (_secretChat->state_->get_id()) {
    case td_api::secretChatStateReady::ID:
        return "ready";
    case td_api::secretChatStateClosed::ID:
        return "closed";
    case td_api::secretChatStatePending::ID:
        return "pending";
    default:
        return "";
    }
}

qint32 Chat::getTtl() const
{
    if (_secretChat == nullptr) return 0;

    return _secretChat->ttl_;
}

qint32 Chat::getMuteFor() const
{
    if (_notificationSettings == nullptr) return 0;

    if (_notificationSettings->use_default_mute_for_) return _scopeNotificationSettings->mute_for_;

    return _notificationSettings->mute_for_;
}

void Chat::setMuteFor(qint32 muteFor)
{
    if (_notificationSettings == nullptr) return;

    auto settings = td_api::make_object<td_api::chatNotificationSettings>(
                _notificationSettings->use_default_mute_for_,
                _notificationSettings->mute_for_,
                _notificationSettings->use_default_sound_,
                _notificationSettings->sound_,
                _notificationSettings->use_default_show_preview_,
                _notificationSettings->show_preview_,
                _notificationSettings->use_default_disable_pinned_message_notifications_,
                _notificationSettings->disable_pinned_message_notifications_,
                _notificationSettings->use_default_disable_mention_notifications_,
                _notificationSettings->disable_mention_notifications_
    );
    settings->use_default_mute_for_ = _scopeNotificationSettings->mute_for_ == muteFor;
    settings->mute_for_ = muteFor;
    _manager->sendQuery(new td_api::setChatNotificationSettings(getId(), move(settings)));
}

bool Chat::getDefaultMuteFor() const
{
    if (_notificationSettings == nullptr) return false;

    return _scopeNotificationSettings->mute_for_ == _notificationSettings->mute_for_;
}

bool Chat::getShowPreview() const
{
    if (_notificationSettings == nullptr) return 0;

    if (_notificationSettings->use_default_show_preview_) return _scopeNotificationSettings->show_preview_;

    return _notificationSettings->show_preview_;
}

void Chat::setShowPreview(bool showPreview)
{
    if (_notificationSettings == nullptr) return;

    auto settings = td_api::make_object<td_api::chatNotificationSettings>(
                _notificationSettings->use_default_mute_for_,
                _notificationSettings->mute_for_,
                _notificationSettings->use_default_sound_,
                _notificationSettings->sound_,
                _notificationSettings->use_default_show_preview_,
                _notificationSettings->show_preview_,
                _notificationSettings->use_default_disable_pinned_message_notifications_,
                _notificationSettings->disable_pinned_message_notifications_,
                _notificationSettings->use_default_disable_mention_notifications_,
                _notificationSettings->disable_mention_notifications_
    );
    settings->use_default_show_preview_ = _scopeNotificationSettings->show_preview_ == showPreview;
    settings->show_preview_ = showPreview;
    _manager->sendQuery(new td_api::setChatNotificationSettings(getId(), move(settings)));
}

bool Chat::getDefaultShowPreview() const
{
    if (_notificationSettings == nullptr) return false;

    return _scopeNotificationSettings->show_preview_ == _notificationSettings->show_preview_;
}

bool Chat::getDisablePinnedMessageNotifications() const
{
    if (_notificationSettings == nullptr) return 0;

    if (_notificationSettings->use_default_disable_pinned_message_notifications_) return _scopeNotificationSettings->disable_pinned_message_notifications_;

    return _notificationSettings->disable_pinned_message_notifications_;
}

void Chat::setDisablePinnedMessageNotifications(bool disablePinnedMessageNotifications)
{
    if (_notificationSettings == nullptr) return;

    auto settings = td_api::make_object<td_api::chatNotificationSettings>(
                _notificationSettings->use_default_mute_for_,
                _notificationSettings->mute_for_,
                _notificationSettings->use_default_sound_,
                _notificationSettings->sound_,
                _notificationSettings->use_default_show_preview_,
                _notificationSettings->show_preview_,
                _notificationSettings->use_default_disable_pinned_message_notifications_,
                _notificationSettings->disable_pinned_message_notifications_,
                _notificationSettings->use_default_disable_mention_notifications_,
                _notificationSettings->disable_mention_notifications_
    );
    settings->use_default_disable_pinned_message_notifications_ = _scopeNotificationSettings->disable_pinned_message_notifications_ == disablePinnedMessageNotifications;
    settings->disable_pinned_message_notifications_ = disablePinnedMessageNotifications;
    _manager->sendQuery(new td_api::setChatNotificationSettings(getId(), move(settings)));
}

bool Chat::getDefaultDisablePinnedMessageNotifications() const
{
    if (_notificationSettings == nullptr) return false;

    return _scopeNotificationSettings->disable_pinned_message_notifications_ == _notificationSettings->disable_pinned_message_notifications_;
}

bool Chat::getDisableMentionNotifications() const
{
    if (_notificationSettings == nullptr) return 0;

    if (_notificationSettings->use_default_disable_mention_notifications_) return _scopeNotificationSettings->disable_mention_notifications_;

    return _notificationSettings->disable_mention_notifications_;
}

void Chat::setDisableMentionNotifications(bool disableMentionNotifications)
{
    if (_notificationSettings == nullptr) return;

    auto settings = td_api::make_object<td_api::chatNotificationSettings>(
                _notificationSettings->use_default_mute_for_,
                _notificationSettings->mute_for_,
                _notificationSettings->use_default_sound_,
                _notificationSettings->sound_,
                _notificationSettings->use_default_show_preview_,
                _notificationSettings->show_preview_,
                _notificationSettings->use_default_disable_pinned_message_notifications_,
                _notificationSettings->disable_pinned_message_notifications_,
                _notificationSettings->use_default_disable_mention_notifications_,
                _notificationSettings->disable_mention_notifications_
    );
    settings->use_default_disable_mention_notifications_ = _scopeNotificationSettings->disable_mention_notifications_ == disableMentionNotifications;
    settings->disable_mention_notifications_ = disableMentionNotifications;
    _manager->sendQuery(new td_api::setChatNotificationSettings(getId(), move(settings)));
}

bool Chat::getDefaultDisableMentionNotifications() const
{
    if (_notificationSettings == nullptr) return false;

    return _scopeNotificationSettings->disable_mention_notifications_ == _notificationSettings->disable_mention_notifications_;
}

qint64 Chat::getPinnedMessageId()
{
    if (_chat->pinned_message_id_ != 0 && !_messages.contains(_chat->pinned_message_id_)) {
        getMessage(_chat->pinned_message_id_);
    }

    return _chat->pinned_message_id_;
}

void Chat::setTelegramManager(shared_ptr<TelegramManager> manager)
{
    _manager = manager;

    connect(manager.get(), SIGNAL(updateChatReadInbox(td_api::updateChatReadInbox*)), this, SLOT(updateChatReadInbox(td_api::updateChatReadInbox*)));
    connect(_manager.get(), SIGNAL(updateChatReadOutbox(td_api::updateChatReadOutbox*)), this, SLOT(updateChatReadOutbox(td_api::updateChatReadOutbox*)));
    connect(_manager.get(), SIGNAL(messages(td_api::messages*)), this, SLOT(messages(td_api::messages*)));
    connect(_manager.get(), SIGNAL(updateNewMessage(td_api::updateNewMessage*)), this, SLOT(updateNewMessage(td_api::updateNewMessage*)));
    connect(_manager.get(), SIGNAL(updateChatTitle(td_api::updateChatTitle*)), this, SLOT(updateChatTitle(td_api::updateChatTitle*)));
    connect(_manager.get(), SIGNAL(updateDeleteMessages(td_api::updateDeleteMessages*)), this, SLOT(updateDeleteMessages(td_api::updateDeleteMessages*)));
    connect(_manager.get(), SIGNAL(updateBasicGroupFullInfo(td_api::updateBasicGroupFullInfo*)), this, SLOT(updateBasicGroupFullInfo(td_api::updateBasicGroupFullInfo*)));
    connect(_manager.get(), SIGNAL(updateSupergroupFullInfo(td_api::updateSupergroupFullInfo*)), this, SLOT(updateSupergroupFullInfo(td_api::updateSupergroupFullInfo*)));
    connect(_manager.get(), SIGNAL(updateSecretChat(td_api::updateSecretChat*)), this, SLOT(updateSecretChat(td_api::updateSecretChat*)));
    connect(_manager.get(), SIGNAL(myIdChanged(qint32)), this, SIGNAL(isSelfChanged()));
    connect(_manager.get(), SIGNAL(updateChatNotificationSettings(td_api::updateChatNotificationSettings*)), this, SLOT(updateChatNotificationSettings(td_api::updateChatNotificationSettings*)));
    connect(_manager.get(), SIGNAL(updateChatPinnedMessage(td_api::updateChatPinnedMessage*)), this, SLOT(updateChatPinnedMessage(td_api::updateChatPinnedMessage*)));
    connect(_manager.get(), SIGNAL(message(td_api::message*)), this, SLOT(gotMessage(td_api::message*)));
}

void Chat::setUsers(shared_ptr<Users> users)
{
    _users = users;
}

void Chat::setFiles(shared_ptr<Files> files)
{
    _files = files;
}

int Chat::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _messages.size();
}

QVariant Chat::data(const QModelIndex &index, int role) const
{
    if (rowCount() <= 0 || index.row() < 0 || index.row() >= rowCount()) return QVariant();

    auto message = _messages[_message_ids[index.row()]];
    switch (role) {
    case MessageRoles::TypeRole:
        return getChatType();
    case MessageRoles::IdRole:
        return QVariant::fromValue(message->getId());
    case MessageRoles::MessageRole:
        if (message->getType() == "messageChatDeleteMember") {
            return tr("%1 left").arg(_users->getUser(message->getDeleteMemberId())->getName());
        } else if (message->getType() == "messageChatAddMembers") {
            auto newUsers = message->getAddMembersIds();
            for(auto userId: newUsers) {
                return tr("%1 joined").arg(_users->getUser(userId)->getName());
            }
        }
        return message->getText();
    case MessageRoles::MessageTypeRole:
        return message->getType();
    case MessageRoles::ReceivedRole:
        return QVariant::fromValue(message->received());
    case MessageRoles::ReplyMessageIdRole:
        return message->replyMessageId();
    case MessageRoles::IsForwardedRole:
        return message->getForwardedInfo() != nullptr;
    case MessageRoles::ForwardUserRole:
    {
        if (message->getForwardedInfo() != nullptr && message->getForwardedInfo()->origin_->get_id() == td_api::messageForwardOriginUser::ID) {
            return static_cast<const td_api::messageForwardOriginUser&>(*message->getForwardedInfo()->origin_).sender_user_id_;
        }
        return 0;
    }
    case MessageRoles::ForwardUsernameRole:
    {
        if (message->getForwardedInfo() != nullptr && message->getForwardedInfo()->origin_->get_id() == td_api::messageForwardOriginHiddenUser::ID) {
            return QString::fromStdString(static_cast<const td_api::messageForwardOriginHiddenUser&>(*message->getForwardedInfo()->origin_).sender_name_);
        }
        return "";
    }
    case MessageRoles::ForwardChannelRole:
    {
        if (message->getForwardedInfo() != nullptr && message->getForwardedInfo()->origin_->get_id() == td_api::messageForwardOriginChannel::ID) {
            return static_cast<const td_api::messageForwardOriginChannel&>(*message->getForwardedInfo()->origin_).chat_id_;
        }
        return 0;
    }
    case MessageRoles::FileRole:
    {
        switch (message->getContentType()) {
        case td_api::messagePhoto::ID:
        {
            auto photo = message->getPhoto();
            QQmlEngine::setObjectOwnership(photo, QQmlEngine::CppOwnership);
            return QVariant::fromValue(photo);
        }
        case td_api::messageSticker::ID:
        {
            auto sticker = message->getSticker();
            QQmlEngine::setObjectOwnership(sticker, QQmlEngine::CppOwnership);
            return QVariant::fromValue(sticker);
        }
        case td_api::messageVideo::ID:
        {
            auto video = message->getVideo();
            QQmlEngine::setObjectOwnership(video, QQmlEngine::CppOwnership);
            return QVariant::fromValue(video);
        }
        case td_api::messageDocument::ID:
        {
            auto document = message->getDocument();
            QQmlEngine::setObjectOwnership(document, QQmlEngine::CppOwnership);
            return QVariant::fromValue(document);
        }
        case td_api::messageAudio::ID:
        {
            auto audio = message->getAudio();
            QQmlEngine::setObjectOwnership(audio, QQmlEngine::CppOwnership);
            return QVariant::fromValue(audio);
        }
        case td_api::messageAnimation::ID:
        {
            auto animation = message->getAnimation();
            QQmlEngine::setObjectOwnership(animation, QQmlEngine::CppOwnership);
            return QVariant::fromValue(animation);
        }
        case td_api::messageVoiceNote::ID:
        {
            auto voiceNote = message->getVoiceNote();
            QQmlEngine::setObjectOwnership(voiceNote, QQmlEngine::CppOwnership);
            return QVariant::fromValue(voiceNote);
        }
        case td_api::messageVideoNote::ID:
        {
            auto videoNote = message->getVideoNote();
            QQmlEngine::setObjectOwnership(videoNote, QQmlEngine::CppOwnership);
            return QVariant::fromValue(videoNote);
        }
        }
        break;
     }
    case MessageRoles::TimeRole:
        return message->getFormattedTimestamp();
    case MessageRoles::AuthorIdRole:
        return message->getSenderUserId();
    case MessageRoles::EditedRole:
        return message->isEdited();
    case MessageRoles::CanBeEditedRole:
        return message->canBeEdited();
    case MessageRoles::CanBeForwardedRole:
        return message->canBeForwarded();
    case MessageRoles::CanBeDeletedRole:
        return message->canBeForwarded();
    case MessageRoles::IsChannelPostRole:
        return message->isChannelPost();
    case MessageRoles::EditedDateRole:
        return message->getEditedDate();
    case MessageRoles::ViewsRole:
        return message->getViews();
    case MessageRoles::ReadRole:
    {
        if (message->received()) {
            return message->getId() <= _lastReadInboxMessageId;
        }  else {
            return message->getId() <= _lastReadOutboxMessageId;
        }
    }
    case MessageRoles::HasWebPageRole:
        return message->hasWebPage();
    case MessageRoles::WebPageRole:
    {
        auto webPage = message->getWebPage();
        QQmlEngine::setObjectOwnership(webPage, QQmlEngine::CppOwnership);
        return QVariant::fromValue(webPage);
    }
    case MessageRoles::PollRole:
    {
        auto poll = message->getPoll();
        QQmlEngine::setObjectOwnership(poll, QQmlEngine::CppOwnership);
        return QVariant::fromValue(poll);
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> Chat::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[IdRole]   = "messageId";
    roles[MessageRole] = "messageText";
    roles[MessageTypeRole] = "messageType";
    roles[ReceivedRole] = "received";
    roles[ReplyMessageIdRole] = "replyMessageId";
    roles[IsForwardedRole] = "isForwarded";
    roles[ForwardUserRole] = "forwardUserId";
    roles[ForwardUsernameRole] = "forwardUsername";
    roles[ForwardChannelRole] = "forwardChannelId";
    roles[FileRole] = "file";
    roles[TimeRole] = "timestamp";
    roles[AuthorIdRole] = "authorId";
    roles[EditedRole] = "edited";
    roles[CanBeEditedRole] = "canBeEdited";
    roles[CanBeForwardedRole] = "canBeForwarded";
    roles[CanBeDeletedRole] = "canBeDeleted";
    roles[IsChannelPostRole] = "isChannel";
    roles[EditedDateRole] = "editedDate";
    roles[ViewsRole] = "views";
    roles[ReadRole] = "isRead";
    roles[HasWebPageRole] = "hasWebPage";
    roles[WebPageRole] = "webPage";
    roles[PollRole] = "poll";
    return roles;
}

td_api::chat *Chat::getChat()
{
    return _chat;
}

int Chat::getMessageIndex(qint64 messageId)
{
    if(rowCount() <= 0) return -1;

    auto it = std::find(_message_ids.begin(), _message_ids.end(), messageId);

    if (it != _message_ids.end()) {
        return std::distance(_message_ids.begin(), it);
    }

    return -1;
}

td_api::message *Chat::getLastMessage()
{
    return _chat->last_message_.get();
}

void Chat::setLastMessage(td_api::object_ptr<td_api::message> lastMessage)
{
    _chat->last_message_ = move(lastMessage);
}

void Chat::newMessage(td_api::object_ptr<td_api::message> message)
{
    if (false == _messages.contains(message->id_)) {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        auto newMessage = new Message();
        newMessage->setTelegramManager(_manager);
        newMessage->setUsers(_users);
        newMessage->setFiles(_files);
        newMessage->setMessage(message.release());
        newMessage->setChatId(this->getId());
        connect(newMessage, SIGNAL(contentChanged(qint64)), this, SLOT(onMessageContentChanged(qint64)));
        connect(newMessage, SIGNAL(messageIdChanged(qint64,qint64)), this, SLOT(onMessageIdChanged(qint64,qint64)));
        _messages[newMessage->getId()] = newMessage;
        _message_ids.append(newMessage->getId());
        endInsertRows();
        if (newMessage->getId() == _chat->pinned_message_id_) emit pinnedMessageIdChanged();
    }
}

std::vector<std::int32_t> Chat::splitToIntVector(QString string, QString separator)
{
    QStringList optionList = string.split(separator);
    std::vector<std::int32_t> vector;
    for (auto option: optionList) {
        vector.push_back(option.toInt());
    }
    return vector;
}

std::vector<string> Chat::splitToVector(QString string, QString separator)
{
    QStringList optionList = string.split(separator);
    std::vector<std::string> vector;
    for (auto option: optionList) {
        vector.push_back(option.toStdString());
    }
    return vector;
}

void Chat::sendMessage(QString message, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageText>();
    messageContent->text_ = td_api::make_object<td_api::formattedText>();
    messageContent->text_->text_ = message.toStdString();
    messageContent->disable_web_page_preview_ = false;
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::getMoreChatHistory()
{
    auto min = std::min_element(_message_ids.begin(), _message_ids.end());

    if (_message_ids.end() != min) {
        auto index = std::distance(_message_ids.begin(), min);

        this->getChatHistory(_message_ids[index]);
    }
}

qint64 Chat::getAuthorByIndex(qint32 index)
{
    if (rowCount() <= 0 || index < 0 || index >= rowCount()) return 0;

    return _messages[_message_ids[index]]->getSenderUserId();
}

void Chat::setMessageAsRead(qint64 messageId)
{
    _manager->sendQuery(new td_api::viewMessages(this->getId(), {messageId}, false));
}

void Chat::sendPhoto(QString path, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessagePhoto>();
    auto inputFile = td_api::make_object<td_api::inputFileLocal>();
    inputFile->path_ = path.toStdString();
    messageContent->photo_ = std::move(inputFile);
    messageContent->caption_ =  td_api::make_object<td_api::formattedText>();
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendFile(QString path, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageDocument>();
    auto inputFile = td_api::make_object<td_api::inputFileLocal>();
    inputFile->path_ = path.toStdString();
    messageContent->document_ = std::move(inputFile);
    messageContent->caption_ =  td_api::make_object<td_api::formattedText>();
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendMusic(QString path, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageAudio>();
    auto inputFile = td_api::make_object<td_api::inputFileLocal>();
    inputFile->path_ = path.toStdString();
    messageContent->audio_ = std::move(inputFile);
    messageContent->caption_ =  td_api::make_object<td_api::formattedText>();
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendVideo(QString path, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageVideo>();
    auto inputFile = td_api::make_object<td_api::inputFileLocal>();
    inputFile->path_ = path.toStdString();
    messageContent->video_ = std::move(inputFile);
    messageContent->caption_ =  td_api::make_object<td_api::formattedText>();
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendPoll(QString question, QStringList options, bool anonymous, bool multipleAnswers, bool quizMode, int validAnswer = -1)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    auto poll = td_api::make_object<td_api::inputMessagePoll>();
    poll->question_ = question.toStdString();

    std::vector<std::string> vectorOptions;
    for (auto option : options) vectorOptions.push_back(option.toStdString());
    poll->options_ = vectorOptions;
    poll->is_anonymous_ = anonymous;
    if (quizMode) {
        poll->type_ = td_api::make_object<td_api::pollTypeQuiz>(validAnswer);
    } else {
        poll->type_ = td_api::make_object<td_api::pollTypeRegular>(multipleAnswers);
    }

    sendMessage->input_message_content_ = std::move(poll);
    _manager->sendQuery(sendMessage);
}

void Chat::sendVoiceNote(QString path, QString waveform, qint64 duration, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageVoiceNote>();
    auto inputFile = td_api::make_object<td_api::inputFileLocal>();
    inputFile->path_ = path.toStdString();
    messageContent->voice_note_ = std::move(inputFile);
    messageContent->waveform_ = waveform.toStdString();
    messageContent->duration_ = static_cast<qint32>(duration/1000);
    messageContent->caption_ =  td_api::make_object<td_api::formattedText>();
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendSticker(qint32 fileId, qint64 replyToMessageId)
{
    auto sendMessage = new td_api::sendMessage();
    sendMessage->chat_id_ = _chat->id_;

    if (replyToMessageId != 0) {
        sendMessage->reply_to_message_id_ = replyToMessageId;
    }

    auto messageContent = td_api::make_object<td_api::inputMessageSticker>();
    auto inputFile = td_api::make_object<td_api::inputFileId>();
    inputFile->id_ = fileId;
    messageContent->sticker_ = std::move(inputFile);
    messageContent->width_ = 512;
    messageContent->height_ = 512;
    sendMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(sendMessage);
}

void Chat::sendForwardedMessages(QStringList forwardedMessages, qint64 forwardedFrom)
{
    forwardedMessages.sort();

    for (QString message : forwardedMessages) {
        auto sendMessage = new td_api::sendMessage();
        sendMessage->chat_id_ = _chat->id_;

        auto messageContent = td_api::make_object<td_api::inputMessageForwarded>();
        messageContent->from_chat_id_ = forwardedFrom;
        messageContent->message_id_ = message.toLongLong();
        sendMessage->input_message_content_ = std::move(messageContent);

        _manager->sendQuery(sendMessage);
    }
}

void Chat::open(QString path)
{
    QDesktopServices::openUrl(QUrl("file://" + path));
}

void Chat::deleteMessage(qint64 messageId)
{
    _manager->sendQuery(new td_api::deleteMessages(_chat->id_, std::vector<qint64>(1, messageId), true));

}

void Chat::editMessageText(qint64 messageId, QString messageText)
{
    auto editMessage = new td_api::editMessageText();
    editMessage->chat_id_ = _chat->id_;
    editMessage->message_id_ = messageId;

    auto messageContent = td_api::make_object<td_api::inputMessageText>();
    messageContent->text_ = td_api::make_object<td_api::formattedText>();
    messageContent->text_->text_ = messageText.toStdString();
    messageContent->disable_web_page_preview_ = false;
    editMessage->input_message_content_ = std::move(messageContent);

    _manager->sendQuery(editMessage);
}

void Chat::chose(qint64 messageId, QString indexes)
{
    auto optionIds = splitToIntVector(indexes, ",");
    auto request = new td_api::setPollAnswer();
    request->chat_id_ = _chat->id_;
    request->message_id_ = messageId;
    request->option_ids_ = optionIds;
    _manager->sendQuery(request);
}

void Chat::openSecretChat()
{
    _manager->sendQuery(new td_api::createNewSecretChat(static_cast<td_api::chatTypeSecret*>(_chat->type_.get())->secret_chat_id_));
}

void Chat::closeSecretChat()
{
    _manager->sendQuery(new td_api::closeSecretChat(static_cast<td_api::chatTypeSecret*>(_chat->type_.get())->secret_chat_id_));
}

void Chat::clearHistory(bool deleteChat)
{
    _manager->sendQuery(new td_api::deleteChatHistory(getId(), deleteChat, true));

}

void Chat::setTtl(qint32 ttl)
{
    _manager->sendQuery(new td_api::sendChatSetTtlMessage(getId(), ttl));
}

void Chat::getChatHistory(qint64 from_message, int limit, bool localOnly)
{
    auto getChatHistoryQuery = new td_api::getChatHistory();
    getChatHistoryQuery->chat_id_ = _chat->id_;
    getChatHistoryQuery->from_message_id_ = from_message;
    getChatHistoryQuery->offset_ = 0;
    getChatHistoryQuery->limit_ = limit;
    getChatHistoryQuery->only_local_ = localOnly;

    _manager->sendQuery(getChatHistoryQuery);
}

void Chat::getMessage(qint64 messageId)
{
    _manager->sendQuery(new td_api::getMessage(getId(), messageId));
}

bool Chat::hasPhoto()
{
    return _chat->photo_ != nullptr;
}

File* Chat::getSmallPhoto()
{
    auto file = _files->getFile(_smallPhotoId);
    if (file == nullptr) return nullptr;
    return file.get();
}

File* Chat::getBigPhoto()
{
    auto file = _files->getFile(_bigPhotoId);
    if (file == nullptr) return nullptr;
    return file.get();
}

void Chat::updateChatReadInbox(td_api::updateChatReadInbox *updateChatReadInbox)
{
    if (updateChatReadInbox->chat_id_ == this->getId()) {
        setUnreadCount(updateChatReadInbox->unread_count_);
        setLastReadInboxMessageId(updateChatReadInbox->last_read_inbox_message_id_);
        emit dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), {ReadRole});
    }
}

void Chat::updateChatReadOutbox(td_api::updateChatReadOutbox *updateChatReadOutbox)
{
    if (updateChatReadOutbox->chat_id_ == this->getId()) {
        setLastReadOutboxMessageId(updateChatReadOutbox->last_read_outbox_message_id_);
        emit dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), {ReadRole});
    }
}

void Chat::messages(td_api::messages *messages)
{
    for(auto &message: messages->messages_) {
        if (message.get() != nullptr && message->chat_id_ == this->getId()) {
            this->newMessage(move(message));
        }
    }
}

void Chat::updateNewMessage(td_api::updateNewMessage *updateNewMessage)
{
    if (updateNewMessage->message_.get() != nullptr && updateNewMessage->message_->chat_id_ == this->getId()) {
        this->newMessage(move(updateNewMessage->message_));
    }
}

void Chat::updateChatTitle(td_api::updateChatTitle *updateChatTitle)
{
    if (updateChatTitle->chat_id_ == getId()) {
        _chat->title_ = updateChatTitle->title_;
        emit titleChanged();
    }
}

void Chat::updateDeleteMessages(td_api::updateDeleteMessages *updateDeleteMessages)
{
    if (updateDeleteMessages->chat_id_ == this->getId()) {
        for (auto messageId : updateDeleteMessages->message_ids_) {
            auto index = getMessageIndex(messageId);
            if (-1 != index) {
                beginRemoveRows(QModelIndex(), index, index);
                delete _messages.take(messageId);
                _message_ids.remove(index);
                endRemoveRows();
            }
        }
    }
}
void Chat::onMessageContentChanged(qint64 messageId)
{
    auto index = getMessageIndex(messageId);

    if (-1 != index) {
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {FileRole, MessageRole, HasWebPageRole, WebPageRole, PollRole});
    }
}

void Chat::onMessageIdChanged(qint64 oldMessageId, qint64 newMessageId)
{
    qDebug() << oldMessageId << " " << newMessageId;
    auto index = getMessageIndex(oldMessageId);
    _message_ids[index] = newMessageId;
    _messages[newMessageId] = _messages.take(oldMessageId);
    emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IdRole});
}

void Chat::updateBasicGroupFullInfo(td_api::updateBasicGroupFullInfo *updateBasicGroupFullInfo)
{
    if (_chat->type_->get_id() == td_api::chatTypeBasicGroup::ID) {
        auto basicGroupId = static_cast<td_api::chatTypeBasicGroup*>(_chat->type_.get())->basic_group_id_;

        if (updateBasicGroupFullInfo->basic_group_id_ == basicGroupId) {
            _basicGroupFullInfo->setBasicGroupFullInfo(std::move(updateBasicGroupFullInfo->basic_group_full_info_));
            emit basicGroupFullInfoChanged();
        }
    }
}

void Chat::updateSupergroupFullInfo(td_api::updateSupergroupFullInfo *updateSupergroupFullInfo)
{
    if (_chat->type_->get_id() == td_api::chatTypeSupergroup::ID) {
        auto supergroupId = static_cast<td_api::chatTypeSupergroup*>(_chat->type_.get())->supergroup_id_;

        if (updateSupergroupFullInfo->supergroup_id_ == supergroupId) {
            _supergroupFullInfo->setSupergroupFullInfo(std::move(updateSupergroupFullInfo->supergroup_full_info_));
            emit supergroupFullInfoChanged();
        }
    }
}

void Chat::updateSecretChat(td_api::updateSecretChat *updateSecretChat)
{
    if (_chat->type_->get_id() == td_api::chatTypeSecret::ID && updateSecretChat->secret_chat_ != nullptr && updateSecretChat->secret_chat_->id_ == getSecretChatId()) {
        _secretChat = updateSecretChat->secret_chat_.release();
        emit secretChatChanged(getId());
        emit ttlChanged(_secretChat->ttl_);
    }
}

void Chat::updateChatNotificationSettings(td_api::updateChatNotificationSettings *updateChatNotificationSettings)
{
    if (updateChatNotificationSettings->chat_id_ == getId()) {
        _notificationSettings = std::move(updateChatNotificationSettings->notification_settings_);
        emit chatNotificationSettingsChanged();
    }
}

void Chat::scopeNotificationSettingsChanged(td_api::scopeNotificationSettings *scopeNotificationSettings)
{
    _scopeNotificationSettings = scopeNotificationSettings;
}

void Chat::updateChatPinnedMessage(td_api::updateChatPinnedMessage *updateChatPinnedMessage)
{
    if (updateChatPinnedMessage->chat_id_ == getId()) {
        _chat->pinned_message_id_ = updateChatPinnedMessage->pinned_message_id_;
    }
}

void Chat::gotMessage(td_api::message *message)
{
    if (message->chat_id_ == getId()) {
        newMessage(td_api::object_ptr<td_api::message>(message));
    }
}
