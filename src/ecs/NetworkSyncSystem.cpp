#include "NetworkSyncSystem.h"
#include "ComponentManager.h"
#include "TransformComponent.h"
#include "NetworkComponent.h"

namespace StrixVerse
{
    namespace ECS
    {
        void NetworkSyncSystem::update(const std::vector<Entity>& entities, float dt)
        {
            for (Entity entity : entities)
            {
                // Check if the entity has both NetworkComponent and TransformComponent.
                if (m_pComponentManager->hasComponent<NetworkComponent>(entity) &&
                    m_pComponentManager->hasComponent<TransformComponent>(entity))
                {
                    auto& net = m_pComponentManager->getComponent<NetworkComponent>(entity);
                    auto& transform = m_pComponentManager->getComponent<TransformComponent>(entity);

                    if (net.isLocalPlayer)
                    {
                        // For the local player, we update the network component with the current transform.
                        // This data will be sent to the server (by some other system).
                        net.x = transform.position.x;
                        net.y = transform.position.y;
                        net.rotation = transform.rotation;
                    }
                    else
                    {
                        // For remote entities, we update the transform from the network data.
                        transform.position.x = net.x;
                        transform.position.y = net.y;
                        transform.rotation = net.rotation;
                    }
                }
            }
        }
    }
}