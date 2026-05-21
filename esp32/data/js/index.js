

function GoToGerUser(){

    window.location.href = "./geruser.html";

}

function GoToGerProd(){

    window.location.href = "./gerprod.html";
    
}

function GoToGerGesto(){

    window.location.href = "./gergestos.html"
}

function GoToCadGesto(){

    window.location.href = "./cadgestos.html"
}

function GoToCadUser(){

    window.location.href = "/caduser.html";

}

function GoToCadProd(){

    window.location.href = "./cadprod.html";
    
}

function GoToIndex(){

  window.location.href = "./index.html";

}

/* modal */

const modal = document.getElementById("modal");
const modalImg = document.getElementById("modal-img");
const closeBtn = document.getElementById("close");

/* ABRIR MODAL */
document.querySelectorAll(".btn-icon").forEach(btn => {
  btn.addEventListener("click", () => {
    const imgSrc = btn.getAttribute("data-img");
    modalImg.src = imgSrc;
    modal.style.display = "flex";
  });
});

/* FECHAR NO X */
closeBtn.addEventListener("click", () => {
  modal.style.display = "none";
});

/* FECHAR CLICANDO FORA */
modal.addEventListener("click", (e) => {
  if (e.target === modal) {
    modal.style.display = "none";
  }
});

/* Salvamento de dados */

