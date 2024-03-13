document.getElementById('UsuarioForm').addEventListener('submit', function(event) {
    event.preventDefault(); // Evita que el formulario se envíe automáticamente
console.log("por aca si pasa");
    var formData = new FormData(this); // Recoge los datos del formulario
console.log(formData);
    // Envía los datos al servidor usando AJAX
    var xhr = new XMLHttpRequest();
    xhr.open('POST', 'insertarMongodb.php', true);
    xhr.onload = function() {
        if (xhr.status === 200) {
            alert('Usuario registrado exitosamente.');
            //document.getElementById('UsuarioForm').reset(); // Reinicia el formulario
        } else {
            alert('Hubo un error al registrar el usuario.');
        }
    };
    xhr.send(formData);
});

document.addEventListener('DOMContentLoaded', function() {
    // Obtener el botón de cancelar
    var cancelButton = document.getElementById('cancelButton');

    // Agregar un evento de clic al botón de cancelar
    cancelButton.addEventListener('click', function() {
        // Redirigir a la siguiente ruta
        window.location.href = '../usuario/usuarios.html';
    });
});