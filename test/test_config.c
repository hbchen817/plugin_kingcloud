#include <stdio.h>
#include <stdlib.h>

#include "config.h"

int main(int argc, char **argv) {
    struct dp_conf_node *root;
    root = conf_parse_file("config_rex.yaml");
    if (root) {
        struct dp_conf_node *val = conf_node_lookup_child(root, "deviceId");
        printf("node name: %s, node value: %s", val->name, val->value);

        val = conf_get_node(root, "functionality.device.dev_ota_progress.request.payload.desc");
        printf("node name: %s, node value: %s", val->name, val->value);

        //conf_dump(root, "");
        conf_free_root(root);
        return 0;
    } else {
        return -1;
    }
}