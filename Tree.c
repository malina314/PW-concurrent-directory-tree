#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Błąd oznaczający, że `target` jest poddrzewem `source`.
#define EINVALIDTARGET -1

// Sprawdza, czy alokacja się powiodła. W przypadku błędu kończy program z kodem 1.
#define CHECK_PTR(p)        \
    do {                    \
        if (p == NULL) {    \
            exit(1);        \
        }                   \
    } while (0)


struct Tree {
    HashMap* subfolders;
};

Tree* tree_new() {
    Tree* node = malloc(sizeof (Tree));
    CHECK_PTR(node);
    node->subfolders = hmap_new();
    CHECK_PTR(node->subfolders);
    return node;
}

void tree_free(Tree* tree) {
     const char* key;
     void* subtree;
     HashMapIterator it = hmap_iterator(tree->subfolders);
     while (hmap_next(tree->subfolders, &it, &key, &subtree)) {
         tree_free(subtree);
     }
    free(tree);
}

// Zwraca wskaźnik na znaleziony węzeł, jeżeli istnieje, wpp. zwraca NULL.
static Tree* tree_find(Tree* tree, const char* path) {
    if (strcmp(path, "/") == 0 || tree == NULL) {
        return tree;
    }
    char* component = malloc(MAX_FOLDER_NAME_LENGTH + 1);
    CHECK_PTR(component);
    path = split_path(path, component);
    Tree* subtree = hmap_get(tree->subfolders, component);
    free(component);
    return tree_find(subtree, path);
}

char* tree_list(Tree* tree, const char* path) {
    if (is_path_valid(path)) {
        tree = tree_find(tree, path);
        if (tree) {
            return make_map_contents_string(tree->subfolders);
        }
    }
    return NULL;
}

int tree_create(Tree* tree, const char* path) {
    if (is_path_valid(path)) {
        char* new_subfolder_name = malloc(MAX_FOLDER_NAME_LENGTH + 1);
        CHECK_PTR(new_subfolder_name);
        char* parent_path = make_path_to_parent(path, new_subfolder_name);
        tree = tree_find(tree, parent_path);
        if (tree) {
            if (hmap_get(tree->subfolders, new_subfolder_name) == NULL) {
                Tree* new_subfolder = tree_new();
                hmap_insert(tree->subfolders, new_subfolder_name, new_subfolder);
                return 0;
            }
            return EEXIST;
        }
        return ENOENT;
    }
    return EINVAL;
}

int tree_remove(Tree* tree, const char* path) {
    if (strcmp(path, "/") == 0) {
        return EBUSY;
    }
    if (is_path_valid(path)) {
        char* subfolder_to_remove = malloc(MAX_FOLDER_NAME_LENGTH + 1);
        CHECK_PTR(subfolder_to_remove);
        char* parent_path = make_path_to_parent(path, subfolder_to_remove);
        tree = tree_find(tree, parent_path);
        if (tree) {
            Tree* to_remove = hmap_get(tree->subfolders, subfolder_to_remove);
            if (to_remove) {
                if (hmap_size(to_remove->subfolders) == 0) {
                    hmap_free(to_remove->subfolders);
                    free(to_remove);
                    hmap_remove(tree->subfolders, subfolder_to_remove);
                    return 0;
                }
                return ENOTEMPTY;
            }
        }
        return ENOENT;
    }
    return EINVAL;
}

// Sprawdza, czy `pre` jest prefiksem `str`.
static inline bool is_prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (strcmp(source, "/") == 0) {
        return EBUSY;
    }
    if (is_prefix(source, target)) {
        return EINVALIDTARGET;
    }
    if (is_path_valid(source) && is_path_valid(target)) {
        char* source_name = malloc(MAX_FOLDER_NAME_LENGTH + 1);
        CHECK_PTR(source_name);
        char* source_parent_path = make_path_to_parent(source, source_name);
        Tree* source_parent_tree = tree_find(tree, source_parent_path);
        char* target_name = malloc(MAX_FOLDER_NAME_LENGTH + 1);
        CHECK_PTR(target_name);
        char* target_parent_path = make_path_to_parent(source, target_name);
        Tree* target_parent_tree = tree_find(tree, target_parent_path);
        if (source_parent_tree && target_parent_tree) {
            Tree* to_move = hmap_get(source_parent_tree->subfolders, source_name);
            if (to_move) {
                if (hmap_get(target_parent_tree->subfolders, target_name) == NULL) {
                    hmap_remove(source_parent_tree->subfolders, source_name);
                    hmap_insert(target_parent_tree->subfolders, target_name, to_move);
                }
                return EEXIST;
            }
        }
        return ENOENT;
    }
    return EINVAL;
}