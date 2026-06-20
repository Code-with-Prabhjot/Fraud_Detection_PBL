#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_ROWS 5000
#define FEATURES 12

typedef struct Node {
    int feature, isLeaf, result;
    struct Node *left, *right;
} Node;

int data[MAX_ROWS][FEATURES + 1];
int rows = 0;

const char* featureNames[FEATURES] = {
    "High Amount", "Odd Hour", "Unrecognized Device", "Foreign Location",
    "Blacklisted IP", "Crypto Transaction", "Behavioral Anomaly",
    "New Beneficiary", "Rapid Transactions", "Weekend",
    "New Account", "Failed Logins"
};

Node* createNode(int feature, int isLeaf, int result) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->feature = feature; node->isLeaf = isLeaf; node->result = result;
    node->left = NULL; node->right = NULL;
    return node;
}

void loadTrainingData(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("PROCESSED:0|FRAUDS:0\n"); exit(1); }
    char line[1024];
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char temp[1024]; strcpy(temp, line);
        temp[strcspn(temp, "\n")] = 0;
        char *p = strtok(temp, ","); if(!p) continue; data[rows][0] = (strcmp(p, "High") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][1] = (strcmp(p, "Odd_Hours") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][2] = (strcmp(p, "Unrecognized") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][3] = (strcmp(p, "Foreign") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][4] = (strcmp(p, "Blacklisted") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][5] = (strcmp(p, "Crypto") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][6] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][7] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][8] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][9] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][10] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][11] = (strcmp(p, "Yes") == 0) ? 1 : 0;
        p = strtok(NULL, ","); if(!p) continue; data[rows][12] = (strcmp(p, "Fraud") == 0) ? 1 : 0;
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

int bestFeature(int indices[], int size, int used[]) {
    double bestGain = -1; int best = -1;
    double parentEntropy = entropy(indices, size);
    for (int f = 0; f < FEATURES; f++) {
        if (used[f]) continue;
        int left[MAX_ROWS], right[MAX_ROWS], l = 0, r = 0;
        for (int i = 0; i < size; i++) {
            if (data[indices[i]][f] == 0) left[l++] = indices[i]; else right[r++] = indices[i];
        }
        if (l == 0 || r == 0) continue;
        double gain = parentEntropy - (((double)l/size)*entropy(left, l) + ((double)r/size)*entropy(right, r));
        if (gain > bestGain) { bestGain = gain; best = f; }
    }
    return best;
}

int majorityLabel(int indices[], int size) {
    int frauds = 0;
    for (int i = 0; i < size; i++) if (data[indices[i]][FEATURES] == 1) frauds++;
    return frauds > size / 2 ? 1 : 0;
}

Node* buildTree(int indices[], int size, int used[]) {
    if (size == 0) return NULL;
    int pure = 1, first = data[indices[0]][FEATURES];
    for (int i = 1; i < size; i++) { if (data[indices[i]][FEATURES] != first) pure = 0; }
    if (pure) return createNode(-1, 1, first);
    
    int feature = bestFeature(indices, size, used);
    if (feature < 0) return createNode(-1, 1, majorityLabel(indices, size));

    Node *root = createNode(feature, 0, -1);
    int left[MAX_ROWS], right[MAX_ROWS], l = 0, r = 0;
    for (int i = 0; i < size; i++) {
        if (data[indices[i]][feature] == 0) left[l++] = indices[i]; else right[r++] = indices[i];
    }
    
    int l_used[FEATURES], r_used[FEATURES];
    memcpy(l_used, used, sizeof(l_used));
    memcpy(r_used, used, sizeof(r_used));
    l_used[feature] = 1; r_used[feature] = 1;

    root->left = buildTree(left, l, l_used); 
    root->right = buildTree(right, r, r_used);
    return root;
}

int predict(Node *root, int input[]) {
    if (root == NULL) return 0;
    if (root->isLeaf) return root->result;
    return (input[root->feature] == 0) ? predict(root->left, input) : predict(root->right, input);
}

void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left); freeTree(root->right); free(root);
}

// Recursively builds a JSON string of the decision tree
void treeToJson(Node *r, char *buf, int bsz) {
    if (!r) { strncat(buf, "null", bsz - strlen(buf) - 1); return; }
    if (r->isLeaf) {
        char leaf[128]; snprintf(leaf, sizeof(leaf), "{\"name\":\"%s\",\"type\":\"leaf\"}", r->result ? "Fraud" : "Safe");
        strncat(buf, leaf, bsz - strlen(buf) - 1); return;
    }
    char node[256]; snprintf(node, sizeof(node), "{\"name\":\"%s\",\"type\":\"split\",\"children\":[", featureNames[r->feature]);
    strncat(buf, node, bsz - strlen(buf) - 1);
    treeToJson(r->left, buf, bsz); 
    strncat(buf, ",", bsz - strlen(buf) - 1);
    treeToJson(r->right, buf, bsz); 
    strncat(buf, "]}", bsz - strlen(buf) - 1);
}

int main(int argc, char *argv[]) {
    loadTrainingData("training_rules.csv");
    int set1[MAX_ROWS], s1 = 0;
    for (int i = 0; i < rows; i++) set1[s1++] = i;
    int used[FEATURES] = {0};
    Node *tree = buildTree(set1, s1, used);

    // NEW FLAG: If Flask asks for the tree, export it and exit immediately
    if (argc > 1 && strcmp(argv[1], "--tree") == 0) {
        char json[524288]; json[0] = 0;
        treeToJson(tree, json, sizeof(json));
        printf("%s\n", json);
        freeTree(tree);
        return 0;
    }

    FILE *batch_fp = fopen("backend_data.csv", "r");
    FILE *alert_fp = fopen("fraud_alerts.log", "w");
    if (!batch_fp || !alert_fp) { printf("PROCESSED:0|FRAUDS:0\n"); return 1; }

    char line[1024]; int total = 0, frauds = 0, input[FEATURES];
    fgets(line, sizeof(line), batch_fp);
    while (fgets(line, sizeof(line), batch_fp)) {
        char temp_line[1024]; strcpy(temp_line, line);
        temp_line[strcspn(temp_line, "\n")] = 0;
        char *txn_id = strtok(temp_line, ",");
        int amount = atoi(strtok(NULL, ",")); int hour = atoi(strtok(NULL, ","));
        char *device = strtok(NULL, ","); char *loc = strtok(NULL, ",");
        char *ip = strtok(NULL, ","); char *cat = strtok(NULL, ",");

        input[0] = (amount > 50000) ? 1 : 0; input[1] = (hour < 6 || hour > 22) ? 1 : 0;
        input[2] = (strcmp(device, "Unrecognized") == 0) ? 1 : 0; input[3] = (strcmp(loc, "Foreign") == 0) ? 1 : 0;
        input[4] = (strcmp(ip, "Blacklisted") == 0) ? 1 : 0; input[5] = (strcmp(cat, "Crypto") == 0) ? 1 : 0;
        input[6]=0; input[7]=0; input[8]=0; input[9]=0; input[10]=0; input[11]=0;

        int result = predict(tree, input);
        int risk = 0;
        if (input[0]) risk+=20; if (input[1]) risk+=10; if (input[3]) risk+=15;
        if (input[4]) risk+=30; if (input[2]) risk+=15; if (input[5]) risk+=10;
        if (risk >= 40) result = 1;

        total++;
        if (result == 1) { frauds++; fprintf(alert_fp, "%s|%d|%02d:00|%s\n", txn_id, amount, hour, cat); }
    }
    
    fclose(batch_fp); fclose(alert_fp);
    printf("PROCESSED:%d|FRAUDS:%d\n", total, frauds);
    freeTree(tree); 
    return 0;
}