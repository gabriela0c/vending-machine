function GoToCadUser(){

    window.location.href =
        "/caduser.html";
}

function GoToIndex(){

    window.location.href =
        "/index.html";
}

window.addEventListener(
    "DOMContentLoaded",
    carregarUsuarios
);

async function carregarUsuarios() {

    const resposta = await fetch(
        "/usuarios"
    );

    const banco = await resposta.json();

    const lista =
        document.getElementById("lista-usuarios");

    lista.innerHTML = "";

    banco.usuarios.forEach((usuario, index) => {

        const div = document.createElement("div");

        div.className =
            index % 2 === 0
            ? "userboxclara"
            : "userboxescura";

        div.innerHTML = `

            <div class="textc">
                <span>${usuario.nome}</span>
            </div>

            <div class="textc">
                <span>
                    ${
                        usuario.maioridade === "sim"
                        ? ">18"
                        : "<18"
                    }
                </span>
            </div>

            <div class="textc">
                <span>
                    R$ ${usuario.saldo}
                </span>
            </div>

            <div class="actions">

                <button
                    class="btn-icon"
                    onclick="editarUsuario('${usuario.nome}')">

                    <img src="/png/editar.png">

                </button>

                <button
                    class="btn-icon"
                    onclick="excluirUsuario('${usuario.nome}')">

                    <img src="/png/excluir.png">

                </button>

            </div>
        `;

        lista.appendChild(div);

    });

}

async function excluirUsuario(nome) {

    const confirmar = confirm(
        `Deseja realmente excluir ${nome}?`
    );

    if (!confirmar) return;

    await fetch(
        `/deletar-usuario?nome=${encodeURIComponent(nome)}`,
        {
            method: "DELETE"
        }
    );

    carregarUsuarios();
}

async function editarUsuario(nome) {
    // Passa o nome do usuário na URL para a página de cadastro saber quem editar
    window.location.href = `/caduser.html?edit=${encodeURIComponent(nome)}`;
}