#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>

#define INFINITY_COST -1

using namespace std;
ILOSTLBEGIN

// Estrutura para armazenar as informações de cada aresta
typedef struct {
    int custo, capacidade_minima, capacidade_maxima;
} Aresta;

// Variáveis globais
int num_vertices, num_arestas;
unordered_map<int, string> rotulos_vertices;  // Mapeia o ID do vértice para seu rótulo (nome)
unordered_map<string, int> ids_vertices;      // Mapeia o nome do vértice para seu ID
unordered_map<int, unordered_map<int, Aresta>> grafo;  // Representação do grafo
unordered_map<int, int> fontes, destinos;     // Mapeia as fontes e destinos com suas demandas

// Função que configura e resolve o modelo CPLEX
void cplex() {
    IloEnv ambiente;  // Ambiente CPLEX

    // Variáveis de decisão
    IloArray<IloNumVarArray> fluxo(ambiente);  // Matriz de variáveis de fluxo
    for (int i = 0; i < num_vertices; i++) {
        fluxo.add(IloNumVarArray(ambiente));  // Adiciona uma linha para cada vértice
        for (int j = 0; j < num_vertices; j++) {
            fluxo[i].add(IloNumVar(ambiente, 0, INT_MAX));  // Variável de fluxo com limites
        }
    }

    // Modelo
    IloModel modelo(ambiente);
    IloExpr sum(ambiente), sum2(ambiente);

    // Função objetivo: Minimizar o custo total de transporte
    sum.clear();
    for (const auto& vertice_origem : grafo) {
        for (const auto& vertice_destino : vertice_origem.second) {
            sum += vertice_destino.second.custo * fluxo[vertice_origem.first][vertice_destino.first];
        }
    }
    modelo.add(IloMinimize(ambiente, sum));  // Adiciona a função objetivo ao modelo

    // Restrições: Oferta
    for (const auto& fonte : fontes) {
        sum.clear();
        for (const auto& destino : grafo[fonte.first]) {
            sum += fluxo[fonte.first][destino.first];
        }
        modelo.add(sum <= fonte.second);  // Restrição: não exceder a oferta
    }

    // Restrições: Demanda
    for (const auto& destino : destinos) {
        sum.clear();
        for (const auto& origem : grafo) {
            if (origem.second.find(destino.first) != origem.second.end()) {
                sum += fluxo[origem.first][destino.first];
            }
        }
        modelo.add(sum >= destino.second);  // Restrição: satisfazer a demanda
    }

    // Restrições de capacidade nas arestas
    for (int i = 0; i < num_vertices; i++) {
        for (int j = 0; j < num_vertices; j++) {
            auto destino = grafo[i].find(j);
            if (destino != grafo[i].end() && destino->second.capacidade_maxima != INFINITY_COST) {
                modelo.add(fluxo[i][destino->first] <= destino->second.capacidade_maxima);
            }
        }
    }

    // Execução
    IloCplex solver(modelo);  // Configura o solver CPLEX

    time_t inicio, fim;
    string status_solucao;

    time(&inicio);
    solver.solve();  // Executa a resolução do modelo
    time(&fim);

    // Resultados
    bool solucao_existe = true;
    switch (solver.getStatus()) {
        case IloAlgorithm::Optimal:
            status_solucao = "Ótima";
            break;
        case IloAlgorithm::Feasible:
            status_solucao = "Factível";
            break;
        default:
            status_solucao = "Sem Solução";
            solucao_existe = false;
    }

    cout << endl << "Status da solução: " << status_solucao << endl << endl;

    if (solucao_existe) {
        IloNum valor_objetivo = solver.getObjValue();

        cout << "Valores das variáveis de decisão: " << endl;
        for (const auto& vertice_origem : grafo) {
            for (const auto& vertice_destino : vertice_origem.second) {
                IloNum valor_fluxo = solver.getValue(fluxo[vertice_origem.first][vertice_destino.first]);
                cout << "Fluxo de " << rotulos_vertices[vertice_origem.first]
                     << " para " << rotulos_vertices[vertice_destino.first]
                     << ": " << valor_fluxo << endl;
            }
        }

        cout << endl << "Função Objetivo Valor = " << valor_objetivo << endl;

        std::cout << std::fixed;
        std::cout << std::setprecision(6);
        cout << "(" << difftime(fim, inicio) << " segundos)" << endl;
    }

    // Liberação de memória
    sum.end();
    sum2.end();
    modelo.end();
    ambiente.end();
}

// Função principal
int main() {
    // Lê o número de vértices e arestas
    cin >> num_vertices >> num_arestas;

    // Verificação para garantir que os valores foram lidos corretamente
    cout << "Num. vertices: " << num_vertices << ", Num. arestas: " << num_arestas << endl;

    // Lê os vértices (origens e destinos)
    string tipo_vertice;
    for (int i = 0; i < num_vertices; i++) {
        cin >> rotulos_vertices[i];  // Lê o rótulo do vértice
        ids_vertices[rotulos_vertices[i]] = i;

        cin >> tipo_vertice;
        if (tipo_vertice == "origem") {
            cin >> fontes[i];  // Armazena a oferta de uma fonte
        } else if (tipo_vertice == "destino") {
            cin >> destinos[i];  // Armazena a demanda de um destino
        }

        // Verificação de cada vértice e seu tipo
        cout << "Vértice " << rotulos_vertices[i] << ": " << tipo_vertice;
        if (tipo_vertice == "origem") {
            cout << ", oferta: " << fontes[i] << endl;
        } else if (tipo_vertice == "destino") {
            cout << ", demanda: " << destinos[i] << endl;
        }
    }

    // Lê as arestas
    string vertice_origem, vertice_destino;
    for (int i = 0; i < num_arestas; i++) {
        Aresta nova_aresta;
        cin >> vertice_origem >> vertice_destino >> nova_aresta.custo >> nova_aresta.capacidade_minima >> nova_aresta.capacidade_maxima;

        int id_origem = ids_vertices[vertice_origem];
        int id_destino = ids_vertices[vertice_destino];

        grafo[id_origem][id_destino] = nova_aresta;
        grafo[id_destino];  // Inicializa a entrada para garantir a presença no grafo

        // Verificação de cada aresta lida
        cout << "Aresta de " << vertice_origem << " para " << vertice_destino
             << ", Custo: " << nova_aresta.custo
             << ", Capacidade mínima: " << nova_aresta.capacidade_minima
             << ", Capacidade máxima: " << nova_aresta.capacidade_maxima << endl;
    }

    // Executa o CPLEX
    cplex();
    return 0;
}
