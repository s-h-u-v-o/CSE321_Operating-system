#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 5
#define MAX_RESOURCES 5
#define MAX_NAME_LEN 20

// Permissions
typedef enum {
    READ = 1,
    WRITE = 2,
    EXECUTE = 4
} Permission;

// User and Resource definitions
typedef struct {
    char name[MAX_NAME_LEN];
} User;

typedef struct {
    char name[MAX_NAME_LEN];
} Resource;

// ACL Entry
typedef struct {
    char username[MAX_NAME_LEN];
    int permissions;
} ACLEntry;

typedef struct {
    Resource resource;
    ACLEntry aclEntries[MAX_USERS];
    int aclCount;
} ACLControlledResource;

// Capability Entry
typedef struct {
    char resourceName[MAX_NAME_LEN];
    int permissions;
} Capability;

typedef struct {
    char username[MAX_NAME_LEN];
    Capability capabilities[MAX_RESOURCES];
    int capCount;
} CapabilityUser;

// Utility Functions
int hasPermission(int userPerm, int requiredPerm) {
    return (userPerm & requiredPerm) == requiredPerm;
}

int getACLPermission(ACLControlledResource *res, const char *userName) {
    int i;
    for (i = 0; i < (*res).aclCount; i++) {
        if (strcmp((*res).aclEntries[i].username, userName) == 0) {
            return (*res).aclEntries[i].permissions;
        }
    }
    return -1;
}

int getCapabilityPermission(CapabilityUser *user, const char *resourceName) {
    int i;
    for (i = 0; i < (*user).capCount; i++) {
        if (strcmp((*user).capabilities[i].resourceName, resourceName) == 0) {
            return (*user).capabilities[i].permissions;
        }
    }
    return -1;
}

void addACLEntry(ACLControlledResource *res, const char *username, int perm) {
    if ((*res).aclCount < MAX_USERS) {
        strcpy((*res).aclEntries[(*res).aclCount].username, username);
        (*res).aclEntries[(*res).aclCount].permissions = perm;
        (*res).aclCount = (*res).aclCount + 1;
    }
}

void addCapability(CapabilityUser *user, const char *resourceName, int perm) {
    if ((*user).capCount < MAX_RESOURCES) {
        strcpy((*user).capabilities[(*user).capCount].resourceName, resourceName);
        (*user).capabilities[(*user).capCount].permissions = perm;
        (*user).capCount = (*user).capCount + 1;
    }
}

// --- One-line ACL check ---
void checkACLAccess(ACLControlledResource *res, const char *userName, int perm) {
    int userPerm = getACLPermission(res, userName);
    char permStr[50] = "";

    if (perm & READ) strcat(permStr, "READ");
    if (perm & WRITE) strcat(permStr, "WRITE");
    if (perm & EXECUTE) strcat(permStr, "EXECUTE");

    if (userPerm == -1) {
        printf("ACL Check: User %s has NO entry for resource %s: Access DENIED\n",
               userName, (*res).resource.name);
    } else if (hasPermission(userPerm, perm)) {
        printf("ACL Check: User %s requests %s on %s: Access GRANTED\n",
               userName, permStr, (*res).resource.name);
    } else {
        printf("ACL Check: User %s requests %s on %s: Access DENIED\n",
               userName, permStr, (*res).resource.name);
    }
}

// --- One-line Capability check ---
void checkCapabilityAccess(CapabilityUser *user, const char *resourceName, int perm) {
    int userPerm = getCapabilityPermission(user, resourceName);
    char permStr[50] = "";

    if (perm & READ) strcat(permStr, "READ");
    if (perm & WRITE) strcat(permStr, "WRITE");
    if (perm & EXECUTE) strcat(permStr, "EXECUTE");

    if (userPerm == -1) {
        printf("Capability Check: User %s has NO capability for resource %s: Access DENIED\n",
               (*user).username, resourceName);
    } else if (hasPermission(userPerm, perm)) {
        printf("Capability Check: User %s requests %s on %s: Access GRANTED\n",
               (*user).username, permStr, resourceName);
    } else {
        printf("Capability Check: User %s requests %s on %s: Access DENIED\n",
               (*user).username, permStr, resourceName);
    }
}

int main() {
    int i;

    // Users
    User users[MAX_USERS] = {{"Alice"}, {"Bob"}, {"Charlie"}, {"Me"}, {"You"}};

    // Resources
    Resource resources[MAX_RESOURCES] = {{"File1"}, {"File2"}, {"File3"}, {"File4"}, {"File5"}};

    // ACL Setup
    ACLControlledResource aclResources[MAX_RESOURCES];
    for (i = 0; i < MAX_RESOURCES; i++) {
        (*(&aclResources[i])).resource = resources[i];
        (*(&aclResources[i])).aclCount = 0;
    }

    // ACL Entries
    addACLEntry(&aclResources[0], "Alice", READ | WRITE);
    addACLEntry(&aclResources[0], "Bob", READ);
    addACLEntry(&aclResources[3], "Charlie", READ | EXECUTE);
    addACLEntry(&aclResources[2], "Alice", READ);
    addACLEntry(&aclResources[1], "You", WRITE);
    addACLEntry(&aclResources[2], "Me", READ | WRITE | EXECUTE);
    addACLEntry(&aclResources[4], "Bob", WRITE);
    addACLEntry(&aclResources[4], "You", READ);

    // Capability Setup
    CapabilityUser capUsers[MAX_USERS];
    for (i = 0; i < MAX_USERS; i++) {
        strcpy((*(&capUsers[i])).username, (*(&users[i])).name);
        (*(&capUsers[i])).capCount = 0;
    }

    // Capabilities
    addCapability(&capUsers[0], "File1", READ | WRITE);
    addCapability(&capUsers[0], "File3", READ);
    addCapability(&capUsers[1], "File1", READ);
    addCapability(&capUsers[1], "File5", WRITE);
    addCapability(&capUsers[2], "File2", READ | EXECUTE);
    addCapability(&capUsers[3], "File4", READ | WRITE | EXECUTE);
    addCapability(&capUsers[4], "File3", WRITE);
    addCapability(&capUsers[4], "File5", READ);

    // --- ACL Tests ---
    printf("\n--- ACL Tests ---\n");
    checkACLAccess(&aclResources[0], "Alice", READ);
    checkACLAccess(&aclResources[0], "Bob", WRITE);
    checkACLAccess(&aclResources[0], "Charlie", READ);
    checkACLAccess(&aclResources[2], "You", WRITE);
    checkACLAccess(&aclResources[3], "Me", EXECUTE);
    checkACLAccess(&aclResources[4], "Alice", READ);
    checkACLAccess(&aclResources[4], "You", READ);
    checkACLAccess(&aclResources[1], "Bob", READ);
    checkACLAccess(&aclResources[2], "Alice", WRITE);

    // --- Capability Tests ---
    printf("\n--- Capability Tests ---\n");
    checkCapabilityAccess(&capUsers[0], "File1", WRITE);
    checkCapabilityAccess(&capUsers[1], "File1", WRITE);
    checkCapabilityAccess(&capUsers[2], "File1", WRITE);
    checkCapabilityAccess(&capUsers[2], "File2", EXECUTE);
    checkCapabilityAccess(&capUsers[3], "File4", READ);
    checkCapabilityAccess(&capUsers[4], "File3", READ);
    checkCapabilityAccess(&capUsers[4], "File5", READ);
    checkCapabilityAccess(&capUsers[1], "File3", WRITE);
    checkCapabilityAccess(&capUsers[3], "File5", EXECUTE);

    return 0;
}