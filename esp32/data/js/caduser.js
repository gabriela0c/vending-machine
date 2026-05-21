function GoToGerUser(){

    window.location.href = "./geruser.html";

}
function GoToIndex(){

  window.location.href = "./index.html";

}
window.addEventListener("DOMContentLoaded", () => {

    console.log("js carregado");

    const form =
        document.getElementById("formuser");

    console.log(form);

    form.addEventListener("submit", async (e) => {

        console.log("submit iniciou");

        e.preventDefault();

        const dados = {

            nome:
                document.getElementById("nome").value,

            cpf:
                document.getElementById("cpf").value,

            senha:
                document.getElementById("senha").value,
            
            saldo:
                Number(document.getElementById("saldo").value),

            maioridade:
                document.querySelector(
                    'input[name="maioridade"]:checked'
                ).value
        };

        try {

            console.log("enviando fetch");

            const resposta = await fetch(
                "/salvar-usuario",
                {
                    method: "POST",
                    headers: {
                        "Content-Type": "application/json"
                    },
                    body: JSON.stringify(dados)
                }
            );

            console.log("fetch respondeu");

            const texto = await resposta.text();

            console.log(texto);

            alert("Cadastro completo");


            GoToIndex();
            
        }
        catch(err) {

            console.error(err);

        }

    });

});

