#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_ROWS 500
#define FEATURES 6

typedef struct Node {
    int feature, isLeaf, result;
    struct Node *left, *right;
} Node;

int data[MAX_ROWS][FEATURES + 1];
int rows = 0;

Node* createNode(int feature, int isLeaf, int result) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->feature = feature; node->isLeaf = isLeaf; node->result = result;
    node->left = NULL; node->right = NULL;
    return node;
}

/* 1. DYNAMIC TRAINING MODULE: Reads text from external CSV and learns */
void loadTrainingData(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("TRAINING DATA MISSING\n"); exit(1); }
    char line[200];
    fgets(line, sizeof(line), fp); // skip header
    
    while (fgets(line, sizeof(line), fp)) {
        char temp[200]; strcpy(temp, line);
        temp[strcspn(temp, "\n")] = 0;
        
        char *p = strtok(temp, ","); if(!p) continue; data[rows][0] = (strcmp(p, "High") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][1] = (strcmp(p, "Odd_Hours") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][2] = (strcmp(p, "Unrecognized") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][3] = (strcmp(p, "Foreign") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][4] = (strcmp(p, "Blacklisted") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][5] = (strcmp(p, "Crypto") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][6] = (strcmp(p, "Fraud") == 0) ? 1 : 0;
        rows++;
    }
    fclose(fp);
}

double entropy(int indices[], int size) {
    int fraud = 0, safe = 0;
    for (int i = 0; i < size; i++) { if (data[indices[i]][FEATURES] == 1) fraud++; else safe++; }
    if (fraud == 0 || safe == 0) return 0;
    double p1 = (double)fraud / size, p0 = (double)safe / size;
    return -(p1 * log2(p1) + p0 * log2(p0));
}

int bestFeature(int indices[], int size) {
    double bestGain = -1; int best = -1;
    double parentEntropy = entropy(indices, size);
    for (int f = 0; f < FEATURES; f++) {
        int left[MAX_ROWS], right[MAX_ROWS], l = 0, r = 0;
        for (int i = 0; i < size; i++) {
            if (data[indices[i]][f] == 0) left[l++] = indices[i]; else right[r++] = indices[i];
        }
        double gain = parentEntropy - (((double)l/size)*entropy(left, l) + ((double)r/size)*entropy(right, r));
        if (gain > bestGain) { bestGain = gain; best = f; }
    }
    return best;
}

Node* buildTree(int indices[], int size) {
    if (size == 0) return NULL;
    int pure = 1, first = data[indices[0]][FEATURES];
    for (int i = 1; i < size; i++) { if (data[indices[i]][FEATURES] != first) pure = 0; }
    if (pure) return createNode(-1, 1, first);
    
    int feature = bestFeature(indices, size);
    Node *root = createNode(feature, 0, -1);
    int left[MAX_ROWS], right[MAX_ROWS], l = 0, r = 0;
    for (int i = 0; i < size; i++) {
        if (data[indices[i]][feature] == 0) left[l++] = indices[i]; else right[r++] = indices[i];
    }
    root->left = buildTree(left, l); root->right = buildTree(right, r);
    return root;
}

int predict(Node *root, int input[]) {
    if (root == NULL) return 0;
    if (root->isLeaf) return root->result;
    return (input[root->feature] == 0) ? predict(root->left, input) : predict(root->right, input);
}

void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left); 
    freeTree(root->right);
    free(root);
}

/* 2. DYNAMIC BATCH PROCESSOR: Reads exact raw data from backend and evaluates */
int main() {
    // Phase 1: Train the AI dynamically
    loadTrainingData("training_rules.csv");
    int set1[MAX_ROWS], s1 = 0;
    for (int i = 0; i < rows; i++) set1[s1++] = i;
    Node *tree = buildTree(set1, s1);

    // Phase 2: Open Company Backend Data
    FILE *batch_fp = fopen("backend_data.csv", "r");
    FILE *alert_fp = fopen("fraud_alerts.log", "w");
    
    if (!batch_fp || !alert_fp) return 1;

    char line[200];
    int total = 0, frauds = 0, input[6];
    
    fgets(line, sizeof(line), batch_fp); // skip header
    
    while (fgets(line, sizeof(line), batch_fp)) {
        char temp_line[200];
        strcpy(temp_line, line);
        temp_line[strcspn(temp_line, "\n")] = 0;
        
        char *txn_id = strtok(temp_line, ",");
        int amount = atoi(strtok(NULL, ","));
        int hour = atoi(strtok(NULL, ","));
        char *device = strtok(NULL, ",");
        char *loc = strtok(NULL, ",");
        char *ip = strtok(NULL, ",");
        char *cat = strtok(NULL, ",");

        // Translate exact numbers into AI rule formats
        input[0] = (amount > 50000) ? 1 : 0;
        input[1] = (hour < 6 || hour > 22) ? 1 : 0;
        input[2] = (strcmp(device, "Unrecognized") == 0) ? 1 : 0;
        input[3] = (strcmp(loc, "Foreign") == 0) ? 1 : 0;
        input[4] = (strcmp(ip, "Blacklisted") == 0) ? 1 : 0;
        input[5] = (strcmp(cat, "Crypto") == 0) ? 1 : 0;

        int result = predict(tree, input);
        total++;
        if (result == 1) {
            frauds++;
            fprintf(alert_fp, "%s|%d|%02d:00|%s\n", txn_id, amount, hour, cat);
        }
    }
    
    fclose(batch_fp); 
    fclose(alert_fp);
    
    printf("PROCESSED:%d|FRAUDS:%d\n", total, frauds);
    
    // Phase 3: Free Memory (Prevents memory leaks)
    freeTree(tree); 
    
    return 0;
}