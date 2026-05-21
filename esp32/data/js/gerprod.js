
function GoToCadProd(){

    window.location.href = "./cadprod.html";
    
}

function GoToIndex(){

  window.location.href = "./index.html";

}

function GoToGerProd(){

    window.location.href = "./gerprod.html";
    
}
window.addEventListener("DOMContentLoaded", carregarProdutos);

async function carregarProdutos() {

    const resposta = await fetch(
        "/produtos"
    );

    const banco = await resposta.json();

    const lista =
        document.getElementById("lista-produtos");

    lista.innerHTML = "";

    banco.produtos.forEach((produto, index) => {

        const div = document.createElement("div");

        // alterna clara/escura
        div.className =
            index % 2 === 0
            ? "userboxclara"
            : "userboxescura";

        div.innerHTML = `

            <div class="textc">
                <span>${produto.nome}</span>
            </div>

            <div class="textc">
                <span>
                    ${
                        produto.maioridade === "sim"
                        ? ">18"
                        : "<18"
                    }
                </span>
            </div>

            <div class="actions">

                <button
                    class="btn-icon"
                    onclick="editarProduto('${produto.nome}')">

                    <img src="png/editar.png">

                </button>

                <button
                    class="btn-icon"
                    onclick="excluirProduto('${produto.nome}')">

                    <img src="png/excluir.png">

                </button>

            </div>
        `;

        lista.appendChild(div);

    });

}
async function excluirProduto(nome) {

    const confirmar = confirm(
        `Deseja realmente excluir ${nome}?`
    );

    if (!confirmar) return;
    
    await fetch(
        `/deletar-produto?nome=${encodeURIComponent(nome)}`,
        {
            method: "DELETE"
        }
    );

    carregarProdutos();
    GoToGerProd();
}

async function editarProduto(nome) {

    GoToCadProd();
}

