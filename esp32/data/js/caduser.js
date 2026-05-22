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

            alert("Dados salvos. Prosseguindo para o reconhecimento facial.");

            iniciarReconhecimentoFacial();
            
        }
        catch(err) {

            console.error(err);

        }

    });

});

async function iniciarReconhecimentoFacial() {
    console.log("Pedindo para o ESP32 acionar o Python...");
    
    // O fetch vai "ficar parado" aqui enquanto o Python abre a câmera no PC e processa
    const resposta = await fetch('/solicitar-reconhecimento');
    const resultado = await resposta.json();

    if (resultado.autorizado) {
        alert(`Acesso Liberado! Olá ${resultado.nome}. Saldo: R$ ${resultado.saldo}`);
        window.location.href = "/geruser.html";
    } else {
        alert("Falha no reconhecimento facial ou usuário não cadastrado.");
    }
}

