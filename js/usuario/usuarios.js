
document.addEventListener('DOMContentLoaded', function() {
 
    // Realizar la solicitud GET al servidor para obtener la lista de usuarios
    fetch('lista_usuarios.php', {
        method: 'GET',
        headers: {
            'Content-Type': 'application/json'
        }
    })
    .then(response => response.json())
    .then(data => {
        // Manejar la respuesta (lista de usuarios)
        console.log(data);
        // Aquí puedes procesar los datos y actualizar tu página
        // Por ejemplo, agregar los usuarios a tu tabla HTML
        agregarUsuariosATabla(data);
    })
    .catch(error => {
        // Manejar errores de la solicitud
        console.error('Error al obtener la lista de usuarios:', error);
    });



           

});

function agregarUsuariosATabla(usuarios) {
    // Obtener el cuerpo de la tabla
    var tbody = document.querySelector('tbody');

    // Limpiar cualquier contenido existente en la tabla
    tbody.innerHTML = '';

    // Iterar sobre la lista de usuarios y agregar cada usuario a la tabla
    usuarios.forEach(function(usuario) {
        var row = document.createElement('tr');
        row.innerHTML = `
            <td>${usuario.nombres}</td>
            <td>${usuario.apellidos}</td>
            <td>${usuario.cedula}</td>
            <td>${usuario.celular}</td>
            <td>${usuario.descripcion_estado}</td>
            <td>
                <a href="../usuario/usuario.html?id=${usuario.id_usuario}">
                    <i class="material-icons" style="color: #ff0000">edit</i>
                </a>            
                <a href="#" class="delete-user" data-id="${usuario.id_usuario}">
                    <i class="material-icons" style="color: #ff0000">delete</i>
                </a>

            </td>
        `;
        tbody.appendChild(row);
    });


     // Agregar evento click al icono de eliminar
     document.querySelectorAll('.delete-user').forEach(iconoEliminar => {
        iconoEliminar.addEventListener('click', function(e) {
            e.preventDefault();
            const idUsuario = this.getAttribute('data-id');
            
            // Confirmar si realmente desea eliminar el usuario
            const confirmarEliminar = confirm('¿Está seguro de que desea eliminar este usuario?');
            
            if (confirmarEliminar) {
                // Realizar la solicitud para eliminar el usuario
                fetch(`eliminar_usuario.php?id=${idUsuario}`, {
                    method: 'DELETE'
                })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Error al eliminar el usuario');
                    }
                    return response.json();
                })
                .then(data => {
                    // Manejar la respuesta
                    console.log('Usuario eliminado:', data);
                    document.dispatchEvent(new Event('DOMContentLoaded'));
                    // Actualizar la interfaz de usuario si es necesario
                })
                .catch(error => {
                    // Manejar errores de la solicitud
                    console.error('Error al eliminar el usuario:', error);
                });
            }
        });
    });
}