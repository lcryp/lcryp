#ifndef RULES_DWA_20011020_H
#define RULES_DWA_20011020_H
#include "config.h"
#include "function.h"
#include "mem.h"
#include "modules.h"
#include "timestamp.h"
#include <utility>
typedef struct _rule RULE;
typedef struct _target TARGET;
typedef struct _targets TARGETS;
typedef struct _action ACTION;
typedef struct _actions ACTIONS;
typedef struct _settings SETTINGS;
typedef RULE* rule_ptr;
typedef TARGET* target_ptr;
typedef TARGETS* targets_ptr;
typedef ACTION* action_ptr;
typedef ACTIONS* actions_ptr;
typedef SETTINGS* settings_ptr;
typedef RULE& rule_ref;
typedef TARGET& target_ref;
typedef TARGETS& targets_ref;
typedef ACTION& action_ref;
typedef ACTIONS& actions_ref;
typedef SETTINGS& settings_ref;
using rule_uptr = b2::jam::unique_jptr<_rule>;
using target_uptr = b2::jam::unique_jptr<_target>;
using targets_uptr = b2::jam::unique_jptr<_targets>;
using action_uptr = b2::jam::unique_jptr<_action>;
using actions_uptr = b2::jam::unique_jptr<_actions>;
using settings_uptr = b2::jam::unique_jptr<_settings>;
struct rule_actions {
    int reference_count;
    function_ptr command;
    list_ptr bindlist;
    int flags;
};
#define RULE_NEWSRCS 0x01
#define RULE_TOGETHER 0x02
#define RULE_IGNORE 0x04
#define RULE_QUIETLY 0x08
#define RULE_PIECEMEAL 0x10
#define RULE_EXISTING 0x20
typedef struct rule_actions* rule_actions_ptr;
struct _rule {
    object_ptr name;
    function_ptr procedure;
    rule_actions_ptr actions;
    module_ptr module;
    int exported;
};
struct _actions {
    actions_ptr next;
    actions_ptr tail;
    action_ptr action;
};
struct _action {
    rule_ptr rule;
    targets_uptr targets;
    targets_uptr sources;
    char running;
#define A_INIT 0
#define A_RUNNING_NOEXEC 1
#define A_RUNNING 2
    int refs;
    void* first_cmd;
    void* last_cmd;
};
struct _settings {
    settings_ptr next;
    object_ptr symbol;
    list_ptr value;
};
struct _targets {
    targets_uptr next = nullptr;
    targets_ptr tail = nullptr;
    target_ptr target = nullptr;
    ~_targets()
    {
        targets_uptr sink = std::move(next);
        while ( sink ) sink = std::move(sink->next);
    }
};
struct _target {
    object_ptr name;
    object_ptr boundname;
    actions_ptr actions;
    settings_ptr settings;
    targets_uptr depends;
    targets_uptr dependants;
    targets_uptr rebuilds;
    target_ptr includes;
    timestamp time;
    timestamp leaf;
    short flags;
#define T_FLAG_TEMP 0x0001
#define T_FLAG_NOCARE 0x0002
#define T_FLAG_NOTFILE 0x0004
#define T_FLAG_TOUCHED 0x0008
#define T_FLAG_LEAVES 0x0010
#define T_FLAG_NOUPDATE 0x0020
#define T_FLAG_VISITED 0x0040
#define T_FLAG_RMOLD 0x0080
#define T_FLAG_FAIL_EXPECTED 0x0100
#define T_FLAG_INTERNAL 0x0200
#define T_FLAG_ISFILE 0x0400
#define T_FLAG_PRECIOUS 0x0800
    char binding;
#define T_BIND_UNBOUND 0
#define T_BIND_MISSING 1
#define T_BIND_PARENTS 2
#define T_BIND_EXISTS 3
    char fate;
#define T_FATE_INIT 0
#define T_FATE_MAKING 1
#define T_FATE_STABLE 2
#define T_FATE_NEWER 3
#define T_FATE_SPOIL 4
#define T_FATE_ISTMP 4
#define T_FATE_BUILD 5
#define T_FATE_TOUCHED 5
#define T_FATE_REBUILD 6
#define T_FATE_MISSING 7
#define T_FATE_NEEDTMP 8
#define T_FATE_OUTDATED 9
#define T_FATE_UPDATE 10
#define T_FATE_BROKEN 11
#define T_FATE_CANTFIND 11
#define T_FATE_CANTMAKE 12
    char progress;
#define T_MAKE_INIT 0
#define T_MAKE_ONSTACK 1
#define T_MAKE_ACTIVE 2
#define T_MAKE_RUNNING 3
#define T_MAKE_DONE 4
#define T_MAKE_NOEXEC_DONE 5
#ifdef OPT_SEMAPHORE
#define T_MAKE_SEMAPHORE 5
#endif
    char status;
#ifdef OPT_SEMAPHORE
    target_ptr semaphore;
#endif
    int asynccnt;
    targets_uptr parents;
    target_ptr scc_root;
    target_ptr rescanning;
    int depth;
    char* cmds;
    char const* failed;
};
void action_free(action_ptr);
actions_ptr actionlist(actions_ptr, action_ptr);
void freeactions(actions_ptr);
settings_ptr addsettings(settings_ptr, int flag, object_ptr symbol, list_ptr value);
void pushsettings(module_ptr, settings_ptr);
void popsettings(module_ptr, settings_ptr);
settings_ptr copysettings(settings_ptr);
void freesettings(settings_ptr);
void actions_refer(rule_actions_ptr);
void actions_free(rule_actions_ptr);
rule_ptr bindrule(object_ptr rulename, module_ptr);
rule_ptr import_rule(rule_ptr source, module_ptr, object_ptr name);
void rule_localize(rule_ptr rule, module_ptr module);
rule_ptr new_rule_body(module_ptr, object_ptr rulename, function_ptr func, int exprt);
rule_ptr new_rule_actions(module_ptr, object_ptr rulename, function_ptr command, list_ptr bindlist, int flags);
void rule_free(rule_ptr);
void bind_explicitly_located_targets();
target_ptr bindtarget(object_ptr const);
targets_uptr targetchain(targets_uptr, targets_uptr);
void targetentry(targets_uptr&, target_ptr);
void target_include(target_ptr const including,
    target_ptr const included);
void target_include_many(target_ptr const including,
    list_ptr const included_names);
void targetlist(targets_uptr&, list_ptr target_names);
void touch_target(object_ptr const);
void clear_includes(target_ptr);
target_ptr target_scc(target_ptr);
targets_uptr targets_pop(targets_uptr);
void rules_done();
#endif
