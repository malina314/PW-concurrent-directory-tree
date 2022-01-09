// todo: zwiększyć współbieżność move jak już będzie działało

// todo: poprawić ten komentarz na temat mutexa w rootcie
// autor: Mateusz Malinowski (mm429561)
// W zadaniu występuje problem "czytelnicy i pisarze". Rolę czytelników pełnią
// funkcje `tree_list` i `tree_find`, a rolę pisarzy `tree_create`,
// `tree_remove` i `tree_move`. Do synchronizacji używam zamków, podobnie jak w
// małym zadaniu nr 8. Dodatkowo `tree_move` ingeruje w dwa węzły, więc musi
// założyć dwa zamki. Aby uniknąć zakleszczenia, używam dodatkowego mutexa, w
// celu założenia obu zamków atomowo.

#include "Tree.h"
#include "HashMap.h"
#include "path_utils.h"
#include "ReadWriteLock.h"
#include "checks.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Błąd oznaczający, że `target` jest w poddrzewie `source`.
#define EINVALIDTARGET -1

// Struktura reprezentująca węzeł drzewa.
typedef struct {
    HashMap* subfolders;
    ReadWriteLock lock;
} TreeNode;

struct Tree {
    TreeNode* root;
    ReadWriteLock special_lock;
};

// Tworzy nowy węzeł drzewa.
static inline TreeNode* tree_new_node() {
    TreeNode* node;
    CHECK_PTR(node = malloc(sizeof (TreeNode)));
    CHECK_PTR(node->subfolders = hmap_new());
    CHECK_SYSERR(rwlock_init(&node->lock) != 0);
    return node;
}

Tree* tree_new() {
    Tree* tree;
    CHECK_PTR(tree = malloc(sizeof (Tree)));
    CHECK_SYSERR(rwlock_init(&tree->special_lock) != 0);
    tree->root = tree_new_node();
    return tree;
}

// Zwalnia całą pamięć związaną z danym węzłem.
static inline void tree_free_node(TreeNode* tree_node) {
    const char* key;
    void* subtree;
    HashMapIterator it = hmap_iterator(tree_node->subfolders);
    while (hmap_next(tree_node->subfolders, &it, &key, &subtree)) {
     tree_free_node(subtree);
    }
    hmap_free(tree_node->subfolders);
    CHECK_SYSERR(rwlock_destroy(&tree_node->lock) != 0);
    free(tree_node);
}

void tree_free(Tree* tree) {
    tree_free_node(tree->root);
    CHECK_SYSERR(rwlock_destroy(&tree->special_lock) != 0);
    free(tree);
}

// Zwraca wskaźnik na znaleziony węzeł, jeżeli istnieje, wpp. zwraca NULL.
static TreeNode* tree_find(TreeNode* tree_node, const char* path) {
    if (path == NULL || tree_node == NULL) {
        return NULL;
    }
    if (strcmp(path, "/") == 0) {
        // Zakładamy zamek, który zostanie zwolniony przez funkcję, która
        // wywołała `tree_find`.
        CHECK_SYSERR(rwlock_before_read(&tree_node->lock));
        return tree_node;
    }
    char* component;
    CHECK_PTR(component = malloc(MAX_FOLDER_NAME_LENGTH + 1));
    path = split_path(path, component);
    CHECK_SYSERR(rwlock_before_read(&tree_node->lock));
    TreeNode* subtree = hmap_get(tree_node->subfolders, component);
    CHECK_SYSERR(rwlock_after_read(&tree_node->lock));
    free(component);
    return tree_find(subtree, path);
}

char* tree_list(Tree* tree, const char* path) {
    TreeNode* tree_node = tree->root;
    if (is_path_valid(path)) {
        CHECK_SYSERR(rwlock_before_read(&tree->special_lock));
        tree_node = tree_find(tree_node, path);
        if (tree_node) {
            char* list = make_map_contents_string(tree_node->subfolders);
            CHECK_SYSERR(rwlock_after_read(&tree_node->lock)); // Zwalniamy zamek z `tree_find`).
            CHECK_SYSERR(rwlock_after_read(&tree->special_lock));
            return list;
        }
        CHECK_SYSERR(rwlock_after_read(&tree->special_lock));
    }
    return NULL;
}

int tree_create(Tree* tree, const char* path) {
    TreeNode* tree_node = tree->root;
    if (strcmp(path, "/") == 0) {
        return EEXIST;
    }
    if (is_path_valid(path)) {
        CHECK_SYSERR(rwlock_before_read(&tree->special_lock));
        char* new_subfolder_name;
        CHECK_PTR(new_subfolder_name = malloc(MAX_FOLDER_NAME_LENGTH + 1));
        char* parent_path = make_path_to_parent(path, new_subfolder_name);
        tree_node = tree_find(tree_node, parent_path);
        if (tree_node) {
            CHECK_SYSERR(rwlock_after_read(&tree_node->lock)); // Zwalniamy zamek z `tree_find`.
            CHECK_SYSERR(rwlock_before_write(&tree_node->lock));
            if (hmap_get(tree_node->subfolders, new_subfolder_name) == NULL) {
                TreeNode* new_subfolder = tree_new_node();
                hmap_insert(tree_node->subfolders, new_subfolder_name, new_subfolder);
                CHECK_SYSERR(rwlock_after_write(&tree_node->lock));
                CHECK_SYSERR(rwlock_after_read(&tree->special_lock));
                return 0;
            }
            CHECK_SYSERR(rwlock_after_write(&tree_node->lock));
            CHECK_SYSERR(rwlock_after_read(&tree->special_lock));
            return EEXIST;
        }
        CHECK_SYSERR(rwlock_after_read(&tree->special_lock));
        return ENOENT;
    }
    return EINVAL;
}

int tree_remove(Tree* tree, const char* path) {
    TreeNode* tree_node = tree->root;
    if (strcmp(path, "/") == 0) {
        return EBUSY;
    }
    if (is_path_valid(path)) {
        CHECK_SYSERR(rwlock_before_write(&tree->special_lock));
        char* subfolder_to_remove;
        CHECK_PTR(subfolder_to_remove = malloc(MAX_FOLDER_NAME_LENGTH + 1));
        char* parent_path = make_path_to_parent(path, subfolder_to_remove);
        tree_node = tree_find(tree_node, parent_path);
        if (tree_node) {
            TreeNode* to_remove = hmap_get(tree_node->subfolders, subfolder_to_remove);
            CHECK_SYSERR(rwlock_after_read(&tree_node->lock)); // Zwalniamy zamek z `tree_find`.
            CHECK_SYSERR(rwlock_before_write(&tree_node->lock));
            if (to_remove) {
                // Usuwamy krawędź, żeby nikt więcej nie wszedł to `to_remove`.
                hmap_remove(tree_node->subfolders, subfolder_to_remove);
                CHECK_SYSERR(rwlock_before_write(&to_remove->lock));
                // Mamy zamek i nikt więcej nie wejdzie, więc nikt nie czeka na
                // żadnej zmiennej warunkowej.
                if (hmap_size(to_remove->subfolders) == 0) {
                    hmap_free(to_remove->subfolders);
                    CHECK_SYSERR(rwlock_after_write(&to_remove->lock));
                    CHECK_SYSERR(rwlock_destroy(&to_remove->lock));
                    free(to_remove);
                    CHECK_SYSERR(rwlock_after_write(&tree_node->lock));
                    CHECK_SYSERR(rwlock_after_write(&tree->special_lock));
                    return 0;
                }
                // Nie mogliśmy usunąć, więc przywracamy usuniętą krawędź.
                hmap_insert(tree_node->subfolders, subfolder_to_remove, to_remove);
                CHECK_SYSERR(rwlock_after_write(&to_remove->lock));
                CHECK_SYSERR(rwlock_after_write(&tree_node->lock));
                CHECK_SYSERR(rwlock_after_write(&tree->special_lock));
                return ENOTEMPTY;
            }
            CHECK_SYSERR(rwlock_after_write(&tree_node->lock));
        }
        CHECK_SYSERR(rwlock_after_write(&tree->special_lock));
        return ENOENT;
    }
    return EINVAL;
}

// Sprawdza, czy `pre` jest prefiksem `str`.
static inline bool is_prefix(const char* pre, const char* str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    TreeNode* tree_node = tree->root;
    if (strcmp(source, "/") == 0) {
        return EBUSY;
    }
    if (strcmp(target, "/") == 0) {
        return EEXIST;
    }
    if (is_path_valid(source) && is_path_valid(target)) {
        CHECK_SYSERR(rwlock_before_write(&tree->special_lock));
        char* source_name;
        CHECK_PTR(source_name = malloc(MAX_FOLDER_NAME_LENGTH + 1));
        char* source_parent_path = make_path_to_parent(source, source_name);
        TreeNode* source_parent_tree = tree_find(tree_node, source_parent_path);
        if (source_parent_tree) {
            char* target_name;
            CHECK_PTR(target_name = malloc(MAX_FOLDER_NAME_LENGTH + 1));
            char* target_parent_path = make_path_to_parent(target, target_name);
            TreeNode* target_parent_tree = tree_find(tree_node, target_parent_path);
            if (target_parent_tree) {
                TreeNode* to_move = hmap_get(source_parent_tree->subfolders, source_name);
                CHECK_SYSERR(rwlock_after_read(&source_parent_tree->lock)); // Zwalniamy zamek z `tree_find`.
                CHECK_SYSERR(rwlock_after_read(&target_parent_tree->lock)); // Zwalniamy zamek z `tree_find`.
                CHECK_SYSERR(rwlock_before_write(&source_parent_tree->lock));
                if (source_parent_tree != target_parent_tree) {
                    CHECK_SYSERR(rwlock_before_write(&target_parent_tree->lock));
                }
                int return_value = 0;
                if (to_move) {
                    if (strcmp(source, target) == 0) {
                        return_value = 0;
                    } else if (is_prefix(source, target)) {
                        return_value = EINVALIDTARGET;
                    } else if (hmap_get(target_parent_tree->subfolders, target_name) != NULL) {
                        return_value = EEXIST;
                    } else {
                        hmap_remove(source_parent_tree->subfolders, source_name);
                        hmap_insert(target_parent_tree->subfolders, target_name, to_move);
                    }
                } else {
                    return_value = ENOENT;
                }
                CHECK_SYSERR(rwlock_after_write(&target_parent_tree->lock));
                if (source_parent_tree != target_parent_tree) {
                    CHECK_SYSERR(rwlock_after_write(&source_parent_tree->lock));
                }
                CHECK_SYSERR(rwlock_after_write(&tree->special_lock));
                return return_value;
            }
            CHECK_SYSERR(rwlock_after_read(&source_parent_tree->lock)); // Zwalniamy zamek z `tree_find`.
        }
        CHECK_SYSERR(rwlock_after_write(&tree->special_lock));
        return ENOENT;
    }
    return EINVAL;
}