#include <improbable/c_schema.h>
#include <improbable/c_worker.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Defines

#define ENTITY_ACL_COMPONENT_ID 50
#define POSITION_COMPONENT_ID   54
#define LOGIN_COMPONENT_ID      1000
#define PLAYER_COMPONENT_ID     1001

#define SESSION_ID              1234

#define MAX_COMPONENTS          3
#define MAX_WORKER_ID_SIZE      255

// Global Data [Hack xD]

Worker_EntityId m_entity = 0;
Worker_RequestId m_request = 0;
Worker_ComponentData m_components[MAX_COMPONENTS];
Schema_ComponentData* m_schema[MAX_COMPONENTS];
uint32_t m_component_index = 0;
char m_worker_id[MAX_WORKER_ID_SIZE];

// Operation handlers

void OnAddEntity(const Worker_AddEntityOp* op)
{
    printf("[AddEntity] %" PRId64 "\n", op->entity_id);
}

void OnCommandRequest(Worker_Connection* connection, const Worker_CommandRequestOp* op)
{
    Worker_ComponentId component_id = op->request.component_id;
    Worker_CommandIndex command_index = op->request.command_index;

    printf(
        "[CommandRequest] entity: %" PRId64 " component: %d command: %d\n",
        op->entity_id, component_id, command_index);

    // Login component

    if (component_id == LOGIN_COMPONENT_ID)
    {
        // Login command

        if (command_index == 1)
        {
            // Validate login

            Schema_Object* request_object = Schema_GetCommandRequestObject(op->request.schema_type);
            const uint8_t* username = Schema_GetBytes(request_object, 1);
            uint32_t username_length = Schema_GetBytesLength(request_object, 1);

            const uint8_t* password = Schema_GetBytes(request_object, 2);
            uint32_t password_length = Schema_GetBytesLength(request_object, 2);

            if (memcmp("Username1", username, username_length) != 0 ||
                memcmp("Password1", password, password_length) != 0)
            {
                // Error

                Worker_CommandResponse command_response;
                memset(&command_response, 0, sizeof(command_response));
                command_response.component_id = op->request.component_id;
                command_response.command_index = command_index;
                command_response.schema_type = Schema_CreateCommandResponse();
                Schema_Object* response_object = Schema_GetCommandResponseObject(command_response.schema_type);
                Schema_AddEnum(response_object, 1, 1);
                Worker_Connection_SendCommandResponse(connection, op->request_id, &command_response);
            }
            else
            {
                // EntityAcl

                {
                    assert(m_component_index < MAX_COMPONENTS);
                    m_schema[m_component_index] = Schema_CreateComponentData();
                    Schema_Object* fields_object = Schema_GetComponentDataFields(m_schema[m_component_index]);

                    // read_acl

                    {
                        Schema_Object* read_acl_object = Schema_AddObject(fields_object, 1);

                        {
                            static const char * kAttribute = "server";
                            Schema_Object* attributes = Schema_AddObject(read_acl_object, 1);
                            Schema_AddBytes(attributes, 1, (const uint8_t*) kAttribute, (uint32_t) (sizeof(char) * strlen(kAttribute)));
                        }

                        {
                            static const char * kAttribute = "client";
                            Schema_Object* attributes = Schema_AddObject(read_acl_object, 1);
                            Schema_AddBytes(attributes, 1, (const uint8_t*) kAttribute, (uint32_t) (sizeof(char) * strlen(kAttribute)));
                        }
                    }

                    // component_write_acl

                    {
                        {
                            Schema_Object* component_write_acl_object = Schema_AddObject(fields_object, 2);

                            {
                                Schema_AddUint32(component_write_acl_object, SCHEMA_MAP_KEY_FIELD_ID, ENTITY_ACL_COMPONENT_ID);

                                Schema_Object* value_field_object = Schema_AddObject(component_write_acl_object, SCHEMA_MAP_VALUE_FIELD_ID);
                                static const char * kAttribute = "server";
                                Schema_Object* attributes = Schema_AddObject(value_field_object, 1);
                                Schema_AddBytes(attributes, 1, (const uint8_t*) kAttribute, (uint32_t) (sizeof(char) * strlen(kAttribute)));
                            }
                        }

                        {
                            Schema_Object* component_write_acl_object = Schema_AddObject(fields_object, 2);

                            {
                                Schema_AddUint32(component_write_acl_object, SCHEMA_MAP_KEY_FIELD_ID, POSITION_COMPONENT_ID);

                                Schema_Object* value_field_object = Schema_AddObject(component_write_acl_object, SCHEMA_MAP_VALUE_FIELD_ID);
                                static const char * kAttribute = "server";
                                Schema_Object* attributes = Schema_AddObject(value_field_object, 1);
                                Schema_AddBytes(attributes, 1, (const uint8_t*) kAttribute, (uint32_t) (sizeof(char) * strlen(kAttribute)));
                            }
                        }

                        {
                            Schema_Object* component_write_acl_object = Schema_AddObject(fields_object, 2);

                            {
                                Schema_AddUint32(component_write_acl_object, SCHEMA_MAP_KEY_FIELD_ID, PLAYER_COMPONENT_ID);

                                Schema_Object* value_field_object = Schema_AddObject(component_write_acl_object, SCHEMA_MAP_VALUE_FIELD_ID);
                                Schema_Object* attributes = Schema_AddObject(value_field_object, 1);
                                memset(m_worker_id, 0, MAX_WORKER_ID_SIZE);
                                strcat_s(m_worker_id, MAX_WORKER_ID_SIZE, "workerId:");
                                strcat_s(m_worker_id, MAX_WORKER_ID_SIZE, op->caller_worker_id);
                                Schema_AddBytes(attributes, 1, (const uint8_t*) m_worker_id, (uint32_t) (sizeof(char) * strlen(m_worker_id)));
                            }
                        }
                    }

                    m_components[m_component_index].component_id = ENTITY_ACL_COMPONENT_ID;
                    m_components[m_component_index].reserved = NULL;
                    m_components[m_component_index].schema_type = m_schema[m_component_index];
                    m_components[m_component_index].user_handle = NULL;

                    ++m_component_index;
                }

                // Position

                {
                    assert(m_component_index < MAX_COMPONENTS);
                    m_schema[m_component_index] = Schema_CreateComponentData();
                    Schema_Object* fields_object = Schema_GetComponentDataFields(m_schema[m_component_index]);
                    Schema_Object* position_object = Schema_AddObject(fields_object, 1);
                    Schema_AddDouble(position_object, 1, 5.0f);
                    Schema_AddDouble(position_object, 2, 0.0f);
                    Schema_AddDouble(position_object, 3, 0.0f);

                    m_components[m_component_index].component_id = POSITION_COMPONENT_ID;
                    m_components[m_component_index].reserved = NULL;
                    m_components[m_component_index].schema_type = m_schema[m_component_index];
                    m_components[m_component_index].user_handle = NULL;

                    ++m_component_index;
                }

                // Player

                {
                    assert(m_component_index < MAX_COMPONENTS);
                    m_schema[m_component_index] = Schema_CreateComponentData();
                    Schema_Object* fields_object = Schema_GetComponentDataFields(m_schema[m_component_index]);
                    Schema_Object* player_object = Schema_AddObject(fields_object, 1);
                    (void) player_object; // Player component has no data for now

                    m_components[m_component_index].component_id = PLAYER_COMPONENT_ID;
                    m_components[m_component_index].reserved = NULL;
                    m_components[m_component_index].schema_type = m_schema[m_component_index];
                    m_components[m_component_index].user_handle = NULL;

                    ++m_component_index;
                }

                // Create entity

                Worker_RequestId request_id = Worker_Connection_SendCreateEntityRequest(
                    connection,
                    MAX_COMPONENTS,
                    m_components,
                    NULL,
                    NULL);

                if (request_id == -1)
                {
                    // Error

                    Worker_CommandResponse command_response;
                    memset(&command_response, 0, sizeof(command_response));
                    command_response.component_id = op->request.component_id;
                    command_response.command_index = command_index;
                    command_response.schema_type = Schema_CreateCommandResponse();
                    Schema_Object* response_object = Schema_GetCommandResponseObject(command_response.schema_type);
                    Schema_AddEnum(response_object, 1, 1);
                    Worker_Connection_SendCommandResponse(connection, op->request_id, &command_response);
                }
                else
                {
                    assert(m_request == 0);
                    m_request = request_id;

                    // OK

                    Worker_CommandResponse command_response;
                    memset(&command_response, 0, sizeof(command_response));
                    command_response.component_id = op->request.component_id;
                    command_response.command_index = command_index;
                    command_response.schema_type = Schema_CreateCommandResponse();
                    Schema_Object* response_object = Schema_GetCommandResponseObject(command_response.schema_type);
                    Schema_AddEnum(response_object, 1, 0);
                    Schema_AddUint32(response_object, 2, SESSION_ID);
                    Worker_Connection_SendCommandResponse(connection, op->request_id, &command_response);
                }
            }
        }
    }
}

void OnCommandResponse(Worker_Connection* connection, const Worker_CommandResponseOp* op)
{
    (void) connection;
    Worker_ComponentId component_id = op->response.component_id;
    Worker_CommandIndex command_index = op->response.command_index;

    printf(
        "[CommandResponse] entity: %" PRId64 " component: %d command: %d\n",
        op->entity_id, component_id, command_index);
}

void OnCreateEntityResponse(Worker_Connection* connection, const Worker_CreateEntityResponseOp* op)
{
    if (m_request == op->request_id)
    {
        if (op->status_code == WORKER_STATUS_CODE_SUCCESS)
        {
            assert(m_entity == 0);
            printf("[Entity] Created: %" PRId64 "\n", op->entity_id);
            m_entity = op->entity_id;

            // Send init player command

            Worker_CommandRequest command_request;
            memset(&command_request, 0, sizeof(command_request));
            command_request.component_id = PLAYER_COMPONENT_ID;
            command_request.command_index = 1;
            command_request.schema_type = Schema_CreateCommandRequest();
            Schema_Object* request_object = Schema_GetCommandRequestObject(command_request.schema_type);
            Schema_AddEntityId(request_object, 1, m_entity);

            Worker_CommandParameters command_parameters;
            command_parameters.allow_short_circuit = 0;

            Worker_Connection_SendCommandRequest(
                connection,
                m_entity,
                &command_request,
                NULL,
                &command_parameters);
        }

        m_request = 0;
    }
}

void OnDeleteEntityResponse(const Worker_DeleteEntityResponseOp* op)
{
    if ((m_entity == op->entity_id) && (op->status_code == WORKER_STATUS_CODE_SUCCESS))
    {
        printf("[Entity] Deleted: %" PRId64 "\n", op->entity_id);
        m_entity = 0;
    }
}

void OnDisconnect(const Worker_DisconnectOp* op)
{
    printf("[Disconnect] %s\n", op->reason);
}

void OnEntityQueryResponse(const Worker_EntityQueryResponseOp* op)
{
    printf("[EntityQueryResponse] %" PRId64 "\n", op->request_id);
}

void OnLogMessage(const Worker_LogMessageOp* op)
{
  printf("[Log] %s\n", op->message);
}

void OnRemoveEntity(const Worker_RemoveEntityOp* op)
{
    printf("[RemoveEntity] %" PRId64 "\n", op->entity_id);
}

// Main

int main(int argc, char** argv)
{
    // Check input parameters

    if (argc != 4)
    {
        printf("Usage: %s <hostname> <port> <worker_id>\n", argv[0]);
        printf("Connects to SpatialOS\n");
        printf("    <hostname>      - hostname of the receptionist to connect to.\n");
        printf("    <port>          - port to use\n");
        printf("    <worker_id>     - name of the worker assigned by SpatialOS.\n");
        return EXIT_FAILURE;
    }

    // Direct serialization

    Worker_ComponentVtable default_vtable;
    memset(&default_vtable, 0, sizeof(Worker_ComponentVtable));

    // Connect to spatial os

    Worker_ConnectionParameters params = Worker_DefaultConnectionParameters();
    params.worker_type = "server";
    params.default_component_vtable = &default_vtable;
    Worker_ConnectionFuture* connection_future =
        Worker_ConnectAsync(argv[1], (uint16_t) atoi(argv[2]), argv[3], &params);
    Worker_Connection* connection = Worker_ConnectionFuture_Get(connection_future, NULL);
    Worker_ConnectionFuture_Destroy(connection_future);

    if (!connection)
    {
        printf("[Server] Failed to connect spatial os.\n");
        return EXIT_FAILURE;
    }
    printf("[Server] Succeed to connect spatial os.\n");

    // Main loop

    while (1)
    {
        Worker_OpList* op_list = Worker_Connection_GetOpList(connection, 0);

        for (size_t i = 0; i < op_list->op_count; ++i)
        {
            Worker_Op* op = &op_list->ops[i];
            switch (op->op_type)
            {
                case WORKER_OP_TYPE_DISCONNECT:
                {
                    OnDisconnect(&op->op.disconnect);
                    break;
                }

                case WORKER_OP_TYPE_LOG_MESSAGE:
                {
                    OnLogMessage(&op->op.log_message);
                    break;
                }

                case WORKER_OP_TYPE_ADD_ENTITY:
                {
                    OnAddEntity(&op->op.add_entity);
                    break;
                }

                case WORKER_OP_TYPE_REMOVE_ENTITY:
                {
                    OnRemoveEntity(&op->op.remove_entity);
                    break;
                }

                case WORKER_OP_TYPE_CREATE_ENTITY_RESPONSE:
                {
                    OnCreateEntityResponse(connection, &op->op.create_entity_response);
                    break;
                }

                case WORKER_OP_TYPE_DELETE_ENTITY_RESPONSE:
                {
                    OnDeleteEntityResponse(&op->op.delete_entity_response);
                    break;
                }

                case WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE:
                {
                    OnEntityQueryResponse(&op->op.entity_query_response);
                    break;
                }

                case WORKER_OP_TYPE_COMMAND_REQUEST:
                {
                    OnCommandRequest(connection, &op->op.command_request);
                    break;
                }

                case WORKER_OP_TYPE_COMMAND_RESPONSE:
                {
                    OnCommandResponse(connection, &op->op.command_response);
                    break;
                }

                default:
                {
                    printf("[Command] %" PRId8 "\n", op->op_type);
                    break;
                }
            }
        }

        Worker_OpList_Destroy(op_list);
        Sleep(1);
    }

    // Destroy connection

    Worker_Connection_Destroy(connection);

    // Exit

    return EXIT_SUCCESS;
}
