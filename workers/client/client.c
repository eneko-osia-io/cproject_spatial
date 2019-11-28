#include <improbable/c_schema.h>
#include <improbable/c_worker.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Defines

#define ENTITY_ACL_COMPONENT_ID 50
#define POSITION_COMPONENT_ID   54
#define LOGIN_COMPONENT_ID      1000
#define PLAYER_COMPONENT_ID     1001

// Global Data [Hack xD]

const char* m_username = NULL;
const char* m_password = NULL;
uint32_t m_session_id = 0;

// Operation handlers

void OnAddEntity(const Worker_AddEntityOp* op)
{
    printf("[AddEntity] %" PRId64 "\n", op->entity_id);
}

void OnCommandRequest(Worker_Connection* connection, const Worker_CommandRequestOp* op)
{
    (void) connection;
    Worker_ComponentId component_id = op->request.component_id;
    Worker_CommandIndex command_index = op->request.command_index;

    printf(
        "[CommandRequest] entity: %" PRId64 " component: %d command: %d\n",
        op->entity_id, component_id, command_index);

    // Player component

    if (component_id == PLAYER_COMPONENT_ID)
    {
        // Init command

        if (command_index == 1)
        {
            printf("[PlayerInit] entity: %" PRId64 "\n", op->entity_id);
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

    // Login component

    if (component_id == LOGIN_COMPONENT_ID)
    {
        // Login command

        if (command_index == 1)
        {
            // Check login response

            Schema_Object* response_object = Schema_GetCommandResponseObject(op->response.schema_type);
            uint32_t code = Schema_GetEnum(response_object, 1);

            if (!code)
            {
                m_session_id = Schema_GetUint32(response_object, 2);
            }

            printf("[Login] Result: %s SessionId: %" PRId32 "\n", code == 0 ? "Succeeded" : "Failed", m_session_id);
        }
    }
}

void OnDisconnect(const Worker_DisconnectOp* op)
{
    printf("[Disconnect] %s\n", op->reason);
}

void OnEntityQueryResponse(Worker_Connection* connection, const Worker_EntityQueryResponseOp* op)
{
    printf("[EntityQueryResponse] %" PRId64 "\n", op->request_id);

    if (WORKER_STATUS_CODE_SUCCESS == op->status_code && op->results)
    {
        for (uint32_t i = 0; i < op->result_count; ++i)
        {
            const Worker_Entity* entity = &op->results[i];

            if (entity)
            {
                for (uint32_t k = 0; k < entity->component_count; ++k)
                {
                    if (entity->components[k].component_id == LOGIN_COMPONENT_ID)
                    {
                        // Send login command

                        Worker_CommandRequest command_request;
                        memset(&command_request, 0, sizeof(command_request));
                        command_request.component_id = LOGIN_COMPONENT_ID;
                        command_request.command_index = 1;
                        command_request.schema_type = Schema_CreateCommandRequest();
                        Schema_Object* request_object = Schema_GetCommandRequestObject(command_request.schema_type);
                        Schema_AddBytes(request_object, 1, (const uint8_t*) m_username, (uint32_t) (sizeof(char) * strlen(m_username)));
                        Schema_AddBytes(request_object, 2, (const uint8_t*) m_password, (uint32_t) (sizeof(char) * strlen(m_password)));

                        Worker_CommandParameters command_parameters;
                        command_parameters.allow_short_circuit = 0;

                        Worker_Connection_SendCommandRequest(
                            connection,
                            entity->entity_id,
                            &command_request,
                            NULL,
                            &command_parameters);
                    }
                }
            }
        }
    }
}

void OnLogMessage(const Worker_LogMessageOp* op)
{
  printf("[Log] %s\n", op->message);
}

void OnRemoveEntity(const Worker_RemoveEntityOp* op)
{
    printf("[RemoveEntity] %" PRId64 "\n", op->entity_id);
}

int main(int argc, char** argv)
{
    // Check input parameters

    if (argc != 6)
    {
        printf("Usage: %s <hostname> <port> <worker_id> <username> <password>\n", argv[0]);
        printf("Connects to SpatialOS\n");
        printf("    <hostname>      - hostname of the receptionist to connect to.\n");
        printf("    <port>          - port to use\n");
        printf("    <worker_id>     - name of the worker assigned by SpatialOS.\n");
        printf("    <username>      - login username.\n");
        printf("    <password>      - login password.\n");
        return EXIT_FAILURE;
    }

    // Direct serialization

    Worker_ComponentVtable default_vtable;
    memset(&default_vtable, 0, sizeof(Worker_ComponentVtable));

    // Connect to spatial os

    Worker_ConnectionParameters params = Worker_DefaultConnectionParameters();
    params.worker_type = "client";
    params.network.tcp.multiplex_level = 4;
    params.default_component_vtable = &default_vtable;
    Worker_ConnectionFuture* connection_future =
        Worker_ConnectAsync(argv[1], (uint16_t) atoi(argv[2]), argv[3], &params);
    Worker_Connection* connection = Worker_ConnectionFuture_Get(connection_future, NULL);
    Worker_ConnectionFuture_Destroy(connection_future);

    if (!connection)
    {
        printf("[Client] Failed to connect spatial os.\n");
        return EXIT_FAILURE;
    }
    printf("[Client] Succeed to connect spatial os.\n");

    // Credentials

    m_username = argv[4];
    m_password = argv[5];

    // Query login component entity

    Worker_EntityQuery query;
    query.constraint.constraint_type = WORKER_CONSTRAINT_TYPE_ENTITY_ID;
    query.constraint.constraint.entity_id_constraint.entity_id = 1;
    query.result_type = WORKER_RESULT_TYPE_SNAPSHOT;
    query.snapshot_result_type_component_id_count = 1;
    const Worker_ComponentId query_components[] = { LOGIN_COMPONENT_ID };
	query.snapshot_result_type_component_ids = query_components;
    Worker_Connection_SendEntityQueryRequest(connection, &query, NULL);

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

                case WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE:
                {
                    OnEntityQueryResponse(connection, &op->op.entity_query_response);
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
