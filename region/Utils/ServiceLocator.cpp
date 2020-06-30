#include "ServiceLocator.h"
#include <Networking/MessageHandler.h>

entt::registry* ServiceLocator::_gameRegistry = nullptr;
MessageHandler* ServiceLocator::_selfMessageHandler = nullptr;
MessageHandler* ServiceLocator::_clientMessageHandler = nullptr;

void ServiceLocator::SetRegistry(entt::registry* registry)
{
    assert(_gameRegistry == nullptr);
    _gameRegistry = registry;
}
void ServiceLocator::SetSelfMessageHandler(MessageHandler* messageHandler)
{
    assert(_selfMessageHandler == nullptr);
    _selfMessageHandler = messageHandler;
}
void ServiceLocator::SetClientMessageHandler(MessageHandler* messageHandler)
{
    assert(_clientMessageHandler == nullptr);
    _clientMessageHandler = messageHandler;
}