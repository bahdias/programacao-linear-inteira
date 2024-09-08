/*--------------------- File: main.cpp  ----------------------+
| Modelo PLI - Problema do Caminho Mínimo                     |
| Implementado por Bárbara Pereira Medeiros Dias              |
+-----------------------------------------------------------+ */
#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>

using namespace std;
ILOSTLBEGIN

// Estrutura para armazenar as informações de cada aresta
typedef struct {
    int custo, capacidade_minima, capacidade_maxima;
} Aresta;

// Variáveis globais
int num_vertices, num_arestas;    // Número de vértices e arestas no grafo
unordered_map<int, string> rotulos_vertices;  // Mapeia o ID do vértice para seu rótulo (nome)
unordered_map<string, int> ids_vertices;      // Mapeia o nome do vértice para seu ID
unordered_map<int, unordered_map<int, Aresta>> grafo;  // Representação do grafo (com arestas)
unordered_map<int, int> fontes, destinos;     // Armazena as fontes e destinos do grafo
unordered_set<int> intermediarios;            // Conjunto de vértices intermediários

// Função que define e resolve o problema usando o CPLEX
void cplex() {
    IloEnv ambiente;  // Ambiente CPLEX

    // Variáveis de decisão
    IloArray<IloBoolVarArray> usado(ambiente);
    for (int i = 0; i < num_vertices; i++) {
        usado.add(IloBoolVarArray(ambiente));  // Adiciona um vetor de variáveis booleanas para cada vértice
        for (int j = 0; j < num_vertices; j++) {
            usado[i].add(IloBoolVar(ambiente));  // Cada variável indica se a aresta i->j é usada
        }
    }

    // Modelo
    IloModel modelo(ambiente);
    IloExpr sum(ambiente);   // Expressão para a função objetivo

    // Função objetivo: Minimizar o custo total das arestas usadas
    sum.clear();
    for (auto vertice_origem : grafo) {
        for (auto vertice_destino : vertice_origem.second) {
            // Soma o custo da aresta multiplicado pela variável que indica se a aresta foi usada
            sum += vertice_destino.second.custo * usado[vertice_origem.first][vertice_destino.first];
        }
    }
    modelo.add(IloMinimize(ambiente, sum));   // Define a minimização do custo total como objetivo

    // Restrições: Fluxo saindo da fonte e entrando no destino
    if (!fontes.empty() && !destinos.empty()) {
        int fonte = fontes.begin()->first;
        int destino = destinos.begin()->first;

        // Fluxo saindo da fonte (somente uma aresta pode sair)
        sum.clear();
        for (auto destino_grafo : grafo[fonte]) {
            sum += usado[fonte][destino_grafo.first];
        }
        modelo.add(sum == 1);   // Restrição: deve sair uma aresta da fonte
        cout << "Restrição de fluxo saindo da fonte adicionada." << endl;

        // Fluxo entrando no destino (somente uma aresta pode entrar)
        sum.clear();
        for (auto origem_grafo : grafo) {
            if (origem_grafo.second.find(destino) != origem_grafo.second.end()) {
                sum += usado[origem_grafo.first][destino];
            }
        }
        modelo.add(sum == 1);   // Restrição: deve entrar uma aresta no destino
        cout << "Restrição de fluxo entrando no destino adicionada." << endl;
    }

    // Restrição:Conservação de fluxo para os intermediários
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

        // Restrição: O fluxo que entra no intermediário deve ser igual ao que sai
        modelo.add(sum == sum2);
        cout << "Restrição de fluxo no intermediário " << rotulos_vertices[intermediario] << " adicionada." << endl;
    }

    // Restrições: Capacidade
    for (int i = 0; i < num_vertices; i++) {
        for (int j = 0; j < num_vertices; j++) {
            auto destino = grafo[i].find(j);
            if (destino != grafo[i].end()) {
                modelo.add(usado[i][j] <= 1);  // Garante que uma aresta seja usada no máximo uma vez
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

        cout << endl << "Função Objetivo Valor = " << valor_objetivo << endl;
        
        std::cout << std::fixed;
        std::cout << std::setprecision(6);
        cout << "(" << difftime(fim, inicio) << " segundos)" << endl;
    } else {
        cout << "Nenhuma solução encontrada." << endl;
    }

    // Liberação de memória
    sum.end();
    modelo.end();
    ambiente.end();
}

int main() {
    // Lê o número de vértices e arestas
    cin >> num_vertices >> num_arestas;

    // Lê os vértices (rótulos e tipo: origem, intermediário ou destino)
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

    // Lê as arestas (origem, destino, custo, capacidade mínima, capacidade máxima)
    string vertice_origem, vertice_destino;
    for (int i = 0; i < num_arestas; i++) {
        Aresta nova_aresta;
        cin >> vertice_origem >> vertice_destino >> nova_aresta.custo >> nova_aresta.capacidade_minima >> nova_aresta.capacidade_maxima;

        // Adiciona a aresta ao grafo
        grafo[ids_vertices[vertice_origem]][ids_vertices[vertice_destino]] = nova_aresta;
        grafo[ids_vertices[vertice_destino]];  // Inicializa a entrada para garantir a presença no grafo
    }

    // Executa o CPLEX
    cplex();
    return 0;
}
