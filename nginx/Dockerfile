# Usar una imagen ligera basada en Alpine
FROM nginx:1.30.0-alpine

# Instalar gettext para usar envsubst
RUN apk add --no-cache gettext

# Copiar la plantilla de configuración de Nginx y el script de entrada
COPY default.conf.template /etc/nginx/templates/default.conf.template
COPY entrypoint.sh /entrypoint.sh

# Convertir el script al formato Unix y hacer que el script de entrada sea ejecutable
RUN sed -i 's/\r$//' /entrypoint.sh \
    && chmod +x /entrypoint.sh

# Exponer el puerto 80 para tráfico HTTP
EXPOSE 80

# Usar el script de entrada para inicializar el contenedor
CMD ["./entrypoint.sh"]
