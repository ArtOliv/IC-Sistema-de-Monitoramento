#!/usr/bin/env bash

set -euo pipefail

echo "+++ Instalador para coleta das leituras +++"
echo

# Diretório onde o projeto será configurado
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Entrada do usuário para a coleta de dados e alerta por email
echo "-- Insira as informações --"
echo

read -rp "IP ou host do servidor web (ex: 192.168.0.1): " SERVER_IP
read -rp "Email remetente dos alertas (Gmail): " EMAIL_USER
read -rsp "Senha de aplicativo do Gmail: " EMAIL_PASS
echo

echo
echo "-- Configuração do script --"
echo
echo "O coletor será executado 1 vez por hora."
echo "Dentro dessa execução você deve definir:"
echo
echo "- Quantidade de leituras que deseja realizar"
echo "- Intervalo entre cada leitura"
echo
echo "IMPORTANTE: O tempo total(quantidade x intervalo) não deve passar de 3600 seg(60 min)"
echo "Exemplos válidos: - 6 leituras com intervalo de 600 seg(10 min)"
echo "                  - 12 leituras com intervalo de 300 seg(5 min)"
echo "                  - 5 leituras com intervalo de 120 seg(2 min)"
echo

read -rp "Quantidade de leituras: " TOTAL_LEITURAS
read -rp "Intervalo entre leituras(em segundos): " INTERVALO
read -rp "Quantidade de tentativas de conexão antes de enviar o alerta: " TENTATIVAS
read -rp "OPCIONAL - Email adicionais para o alerta(separados por vírgula): " EXTRA_EMAILS
echo

TOTAL_EXEC_TIME=$(( INTERVALO * TOTAL_LEITURAS ))
if (( TOTAL_EXEC_TIME > 3600 )); then
    echo "ERRO: Tempo total excede 60 minutos. Altere os valores."
    exit 1
fi

if [[ -n "$EXTRA_EMAILS" ]]; then
    EMAILS="$EMAIL_USER,$EXTRA_EMAILS"
else
    EMAILS="$EMAIL_USER"
fi

# Criação e instalação das bibliotecas no ambiente virtual
echo
echo "-- Criando ambiente virtual para o python --"
echo

# Verificação se existe Python 3 instalado
if ! command -v python3 > /dev/null; then
    echo "Python3 não encontrado. Instale antes de continuar."
    exit 1
fi

if [[ ! -d "$BASE_DIR/venv" ]]; then
    python3 -m venv "$BASE_DIR/venv"
fi

source "$BASE_DIR/venv/bin/activate"

pip install --upgrade pip
pip install -r "$BASE_DIR/requirements.txt"

# Criação de pastar para organização do diretório
echo
echo "Criando estrutura de diretórios..."

mkdir -p "$BASE_DIR/Leituras"
mkdir -p "$BASE_DIR/Logs"

# Criação das variáveis de ambiente(arquivo .env)
echo "Configurando variáveis de ambiente..."

cat > "$BASE_DIR/.env" << EOF
URL=$SERVER_IP
EMAIL_USER=$EMAIL_USER
EMAIL_PASS=$EMAIL_PASS
EMAILS=$EMAILS
TOTAL_LEITURAS=$TOTAL_LEITURAS
INTERVALO=$INTERVALO
TENTATIVAS=$TENTATIVAS
BASE_DIR=$BASE_DIR
EOF

chmod 600 "$BASE_DIR/.env"

# Configuração do crontab
echo "Criando cron para execução automática..."

CRON_CMD="$BASE_DIR/venv/bin/python3.12 $BASE_DIR/Coletor.py > /dev/null 2>> $BASE_DIR/Logs/error.log"
CRON_JOB="0 * * * * $CRON_CMD"

# Remove replica desse cron se existir
(crontab -l 2> /dev/null | grep -v "$BASE_DIR/Coletor.py" || true) | crontab -

# Adiciona o novo cron para execução
(crontab -l 2> /dev/null; echo "$CRON_JOB") | crontab -

echo
echo "+++ Instalação concluída com sucesso! +++"