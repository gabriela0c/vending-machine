
function GoToIndex(){

  window.location.href = "./index.html";

}
function GoToGerProd(){

    window.location.href = "./gerprod.html";
    
}

document.getElementById("formprod")
.addEventListener("submit", async (e) => {

    e.preventDefault();

    const dados = {

        nome:
            document.getElementById("nomeprod").value,

        valor:
            Number(document.getElementById("valorprod").value),

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