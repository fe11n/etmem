/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: louhongxiang
 * Create: 2019-12-10
 * Description: Memig project command API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "securec.h"
#include "etmem.h"
#include "etmem_project.h"
#include "etmem_rpc.h"
#include "etmem_common.h"

static void project_help(void)
{
    fprintf(stderr,
            "\nUsage:\n"
            "    etmem project add [options]\n"
            "    etmem project del [options]\n"
            "    etmem project show [options]\n"
            "    etmem project help\n"
            "\nOptions:\n"
            "    -f|--file <conf_file>     Add configuration file\n"
            "    -n|--name <proj_name>     Add project name\n"
            "    -s|--socket <socket_name> Socket name to connect\n"
            "\nNotes:\n"
            "    1. Project name and socket name must be given when execute add or del option.\n"
            "    2. Configuration file must be given when execute add option.\n"
            "    3. Socket name must be given when execute show option.\n");
}

static int project_parse_cmd(const struct etmem_conf *conf, struct mem_proj *proj)
{
    switch (conf->cmd) {
        case ETMEM_CMD_ADD:
            /* fallthrough */
        case ETMEM_CMD_DEL:
            /* fallthrough */
        case ETMEM_CMD_SHOW:
            goto EXIT;
        default:
            printf("invalid command %u of project\n", conf->cmd);
            return -EINVAL;
    }

EXIT:
    proj->cmd = conf->cmd;
    return 0;
}

static int parse_file_name(const char *val, char **file_name)
{
    size_t len;
    int ret;

    len = strlen(val) + 1;
    if (len > FILE_NAME_MAX_LEN) {
        printf("file name too long, should not be larger than %u\n", FILE_NAME_MAX_LEN);
        return -ENAMETOOLONG;
    }

    *file_name = (char *)calloc(len, sizeof(char));
    if (*file_name == NULL) {
        printf("malloc file name failed.\n");
        return -ENOMEM;
    }

    ret = strncpy_s(*file_name, len, val, len - 1);
    if (ret != EOK) {
        printf("strncpy_s file name failed.\n");
        free(*file_name);
        *file_name = NULL;
        return ret;
    }

    return 0;
}

static int project_parse_args(const struct etmem_conf *conf, struct mem_proj *proj)
{
    int opt;
    int ret;
    int params_cnt = 0;
    struct option opts[] = {
        {"file", required_argument, NULL, 'f'},
        {"name", required_argument, NULL, 'n'},
        {"socket", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0},
    };

    while ((opt = getopt_long(conf->argc, conf->argv, "f:n:s:",
                              opts, NULL)) != -1) {
        switch (opt) {
            case 'f':
                ret = parse_file_name(optarg, &proj->file_name);
                if (ret != 0) {
                    printf("parse file name failed.\n");
                    return ret;
                }
                break;
            case 'n':
                ret = parse_name_string(optarg, &proj->proj_name, PROJECT_NAME_MAX_LEN);
                if (ret != 0) {
                    printf("parse project name failed.\n");
                    return ret;
                }
                break;
            case 's':
                ret = parse_name_string(optarg, &proj->sock_name, SOCKET_NAME_MAX_LEN);
                if (ret != 0) {
                    printf("parse socket name failed.\n");
                    return ret;
                }
                break;
            case '?':
                /* fallthrough */
            default:
                printf("invalid option: %s\n", conf->argv[optind]);
                return -EINVAL;
        }
        params_cnt++;
    }

    ret = etmem_parse_check_result(params_cnt, conf->argc);

    return ret;
}

static int project_check_params(const struct mem_proj *proj)
{
    if (proj->sock_name == NULL || strlen(proj->sock_name) == 0) {
        printf("socket name to connect must all be given, please check.\n");
        return -EINVAL;
    }
 
    if (proj->cmd == ETMEM_CMD_SHOW) {
        return 0;
    }

    if (proj->proj_name == NULL || strlen(proj->proj_name) == 0) {
        printf("project name must all be given, please check.\n");
        return -EINVAL;
    }

    if (proj->cmd == ETMEM_CMD_ADD) {
        if (proj->file_name == NULL || strlen(proj->file_name) == 0) {
            printf("file name must be given in add command.\n");
            return -EINVAL;
        }
    }

    return 0;
}

static int project_do_cmd(const struct etmem_conf *conf)
{
    struct mem_proj proj;
    int ret;

    ret = memset_s(&proj, sizeof(struct mem_proj), 0, sizeof(struct mem_proj));
    if (ret != EOK) {
        printf("memset_s for mem_proj failed.\n");
        return ret;
    }

    ret = project_parse_cmd(conf, &proj);
    if (ret != 0) {
        return ret;
    }

    ret = project_parse_args(conf, &proj);
    if (ret != 0) {
        goto EXIT;
    }

    ret = project_check_params(&proj);
    if (ret != 0) {
        goto EXIT;
    }

    etmem_rpc_client(&proj);

EXIT:
    free_proj_member(&proj);
    return ret;
}

static struct etmem_obj g_etmem_project = {
    .name = "project",
    .help = project_help,
    .do_cmd = project_do_cmd,
};

void project_init(void)
{
    etmem_register_obj(&g_etmem_project);
}

void project_exit(void)
{
    etmem_unregister_obj(&g_etmem_project);
}