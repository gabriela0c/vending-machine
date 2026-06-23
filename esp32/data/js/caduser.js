function GoToGerUser(){
    window.location.href = "./geruser.html";
}

function GoToIndex(){
    window.location.href = "./index.html";
}

// CORREÇÃO AQUI: Adicionado 'async' antes de ()
window.addEventListener("DOMContentLoaded", async () => {

    const urlParams = new URLSearchParams(window.location.search);
    const nomeEditar = urlParams.get('edit');

    if (nomeEditar) {
        // 1. Muda o título visual da tela
        document.getElementById("cad").innerText = "EDITAR USUÁRIO";
        document.getElementById("loginbt").innerText = "Salvar Alterações";
        
        // Bloqueia o campo nome se você não quiser que mudem a chave principal
        document.getElementById("nome").value = nomeEditar;
        document.getElementById("nome").disabled = true; 

        try {
            // 2. Busca a lista de usuários para achar os dados do que está sendo editado
            const resposta = await fetch("/usuarios");
            const banco = await resposta.json();
            const usuario = banco.usuarios.find(u => u.nome === nomeEditar);

            if (usuario) {
                // 3. Preenche os campos do formulário com os dados antigos
                document.getElementById("cpf").value = usuario.cpf || "";
                document.getElementById("saldo").value = usuario.saldo;
                
                if (usuario.maioridade === "sim") {
                    document.getElementById("maioridade1").checked = true;
                } else {
                    document.getElementById("maioridade2").checked = true;
                }
            }
        } catch (err) {
            console.error("Erro ao carregar dados para edição:", err);
        }
    }

    console.log("js carregado");

    const form = document.getElementById("formuser");
    console.log(form);

    form.addEventListener("submit", async (e) => {
        console.log("submit iniciou");
        e.preventDefault();

        // Pegando o radio selecionado com segurança para não quebrar o código
        const maioridadeSelecionada = document.querySelector('input[name="maioridade"]:checked');

        const dados = {
            nome: document.getElementById("nome").value,
            cpf: document.getElementById("cpf").value,
            saldo: Number(document.getElementById("saldo").value),
            maioridade: maioridadeSelecionada ? maioridadeSelecionada.value : ""
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

            const urlParams = new URLSearchParams(window.location.search);
            if (urlParams.get('edit')) {
                alert("Usuário atualizado com sucesso!");
                window.location.href = "./geruser.html";
            } else {
                alert("Dados salvos com sucesso! Prepare-se para a captura da foto. Olhe para a câmera");
                iniciarCadastroFacial(dados.nome);
            }
            
        } catch(err) {
            console.error(err);
        }
    });
});

async function iniciarCadastroFacial(nome) {
    console.log("Pedindo para o ESP32 iniciar o cadastro da face no Python...");
    
    try {
        const resposta = await fetch(`/solicitar-cadastro?nome=${encodeURIComponent(nome)}`);
        const resultado = await resposta.json();

        if (resultado.sucesso) {
            alert(`Cadastro concluído com sucesso! Rosto de ${nome} registrado.`);
            window.location.href = "./geruser.html";
        } else {
            alert("Falha ao cadastrar rosto: " + resultado.mensagem);
        }
    } catch (err) {
        console.error(err);
        alert("Erro de comunicação com o servidor ao tentar cadastrar a face.");
    }
}