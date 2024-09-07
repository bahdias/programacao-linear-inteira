/*--------------------- File: main.cpp  ----------------------+
|Modelo PLI - Problema do Caminho Mínimo                      |
|					      		                              |
|					      		                              |
| Implementado por Bárbara Pereira Medeiros dias e            |
|                    Julia Luiza Ferreira Santos              |
+-----------------------------------------------------------+ */
#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>

using namespace std;
ILOSTLBEGIN

typedef struct {
    int custo;
    int capacidade_minima;
    int capacidade_maxima;
} Aresta;

int num_vertices, num_arestas;
unordered_map<int, string> rotulos_vertices;
unordered_map<string, int> ids_vertices;
unordered_map<int, unordered_map<int, Aresta>> grafo;
unordered_map<int, int> fontes, destinos;
unordered_set<int> intermediarios;

void cplex() {
    IloEnv ambiente;

    // Variáveis de decisão
    IloArray<IloBoolVarArray> usado(ambiente);
    for (int i = 0; i < num_vertices; i++) {
        usado.add(IloBoolVarArray(ambiente));
        for (int j = 0; j < num_vertices; j++) {
            usado[i].add(IloBoolVar(ambiente));
        }
    }

    // Modelo
    IloModel modelo(ambiente);
    IloExpr sum(ambiente);

    // Função objetivo: Minimizar o custo total das arestas usadas
    sum.clear();
    for (auto vertice_origem : grafo) {
        for (auto vertice_destino : vertice_origem.second) {
            sum += vertice_destino.second.custo * usado[vertice_origem.first][vertice_destino.first];
        }
    }
    modelo.add(IloMinimize(ambiente, sum));

    // Restrições: Fluxo saindo da fonte e entrando no destino
    if (!fontes.empty() && !destinos.empty()) {
        int fonte = fontes.begin()->first;
        int destino = destinos.begin()->first;

        // Fluxo saindo da fonte
        sum.clear();
        for (auto destino_grafo : grafo[fonte]) {
            sum += usado[fonte][destino_grafo.first];
        }
        modelo.add(sum == 1);
        cout << "Restrição de fluxo saindo da fonte adicionada." << endl;

        // Fluxo entrando no destino
        sum.clear();
        for (auto origem_grafo : grafo) {
            if (origem_grafo.second.find(destino) != origem_grafo.second.end()) {
                sum += usado[origem_grafo.first][destino];
            }
        }
        modelo.add(sum == 1);
        cout << "Restrição de fluxo entrando no destino adicionada." << endl;
    }

    // Restrições: Conservação de fluxo nos intermediários
    for (int intermediario : intermediarios) {
        sum.clear();
        IloExpr sum2(ambiente);

        // Fluxo entrando no intermediário
        for (auto origem : grafo) {
            if (origem.second.find(intermediario) != origem.second.end()) {
                sum += usado[origem.first][intermediario];
            }
        }

        // Fluxo saindo do intermediário
        for (auto destino : grafo[intermediario]) {
            sum2 += usado[intermediario][destino.first];
        }

        modelo.add(sum == sum2);
        cout << "Restrição de fluxo no intermediário " << rotulos_vertices[intermediario] << " adicionada." << endl;
    }

    // Restrições de capacidade (se necessário)
    for (int i = 0; i < num_vertices; i++) {
        for (int j = 0; j < num_vertices; j++) {
            auto destino = grafo[i].find(j);
            if (destino != grafo[i].end()) {
                modelo.add(usado[i][j] <= 1);
            }
        }
    }

    IloCplex solver(modelo);

    time_t inicio, fim;
    string status_solucao;

    time(&inicio);
    solver.solve();
    time(&fim);

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

        cout << "Arestas usadas no caminho mínimo: " << endl;
        for (auto vertice_origem : grafo) {
            for (auto vertice_destino : vertice_origem.second) {
                IloNum valor_usado = solver.getValue(usado[vertice_origem.first][vertice_destino.first]);
                if (valor_usado > 0.5) {
                    cout << "Aresta de " << rotulos_vertices[vertice_origem.first]
                         << " para " << rotulos_vertices[vertice_destino.first]
                         << " usada no caminho" << endl;
                }
            }
        }

        cout << endl << "Custo total do caminho = " << valor_objetivo << endl;

        std::cout << std::fixed;
        std::cout << std::setprecision(6);
        cout << "(" << difftime(fim, inicio) << " segundos)" << endl;
    } else {
        cout << "Nenhuma solução encontrada." << endl;
    }

    sum.end();
    modelo.end();
    ambiente.end();
}

int main() {
    // Lê o número de vértices e arestas
    cin >> num_vertices >> num_arestas;

    // Lê os vértices (origens, destinos e intermediários)
    string tipo_vertice;
    for (int i = 0; i < num_vertices; i++) {
        cin >> rotulos_vertices[i];
        ids_vertices[rotulos_vertices[i]] = i;

        cin >> tipo_vertice;
        if (tipo_vertice == "origem") {
            fontes[i] = i;
        } else if (tipo_vertice == "destino") {
            destinos[i] = i;
        } else {
            intermediarios.insert(i);
        }
    }

    // Lê as arestas
    string vertice_origem, vertice_destino;
    for (int i = 0; i < num_arestas; i++) {
        Aresta nova_aresta;
        cin >> vertice_origem >> vertice_destino >> nova_aresta.custo >> nova_aresta.capacidade_minima >> nova_aresta.capacidade_maxima;

        grafo[ids_vertices[vertice_origem]][ids_vertices[vertice_destino]] = nova_aresta;
        grafo[ids_vertices[vertice_destino]];  // Inicializa a entrada para garantir a presença no grafo
    }

    // Executa o CPLEX
    cplex();
}
