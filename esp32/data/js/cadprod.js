
function GoToIndex(){

  window.location.href = "./index.html";

}
function GoToGerProd(){

    window.location.href = "./gerprod.html";
    
}

window.addEventListener("DOMContentLoaded", () => {
    const produto = JSON.parse(localStorage.getItem("produto_editando"));
    if (produto) {
        document.getElementById("nomeprod").value = produto.nome;
        document.getElementById("valorprod").value = produto.valor;
        document.getElementById("estoqueprod").value = produto.estoque || 0;
        
        if (produto.maioridade === "sim") {
            document.getElementById("maioridade1").checked = true;
        } else {
            document.getElementById("maioridade2").checked = true;
        }
        
        // Bloqueia o nome para não criar duplicatas se quiser apenas editar
        // document.getElementById("nomeprod").disabled = true; 
        
        // Limpa para não preencher em um novo cadastro futuro
        localStorage.removeItem("produto_editando");
    }
});

document.getElementById("formprod")
.addEventListener("submit", async (e) => {

    e.preventDefault();

    const dados = {

        nome:
            document.getElementById("nomeprod").value,

        valor:
            Number(document.getElementById("valorprod").value),

        estoque:
            Number(document.getElementById("estoqueprod").value),

        maioridade:
            document.querySelector(
                'input[name="maioridade"]:checked'
            ).value
    };

    try {

        const resposta = await fetch(
            "/salvar-produto",
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify(dados)
            }
        );

        const texto = await resposta.text();

        alert(texto);
        GoToGerProd();

    }
    catch(err) {

        console.error(err);

    }
    
});