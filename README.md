# IC-Sistema-de-Monitoramento

# Descrição Geral

Este projeto de **Iniciação Científica** consiste no desenvolvimento de um sistema embarcado para monitoramento de estufas agrícolas no Campus Tancredo Neves da UFSJ.

A solução foi construída utilizando um **Arduino MEGA** para aquisição dos dados dos sensores e um **ESP8266** para conectividade e disponibilização das informações via servidor web local.

O sistema realiza a medição contínua de:

- **Temperatura ambiente**
- **Umidade do ar**
- **Umidade do solo**
- **Luminosidade**

Além da visualização em tempo real via navegador, o projeto inclui um coletor automatizado em **Python**, responsável por extrair periodicamente os dados do servidor web e armazená-los em formato **CSV** para análises futuras, e um **Dashboard** que extrai dos arquivos **CSV** os dados para plotar em um gráfico.

---

## Motivação

A agricultura em ambientes controlados, como estufas, exige monitoramento constante de variáveis ambientais. Pequenas variações em temperatura, umidade ou luminosidade podem impactar diretamente no desenvolvimento das plantas.

Este projeto surge como uma solução de baixo custo, escalável e replicável, permitindo:

- **Monitoramento contínuo**
- **Registro estruturado dos dados**
- **Base para análises estatísticas e modelagem futura**

---

## Estrutura do Projeto

```
.
├── Coletor/
│ ├── Coletor.py
| ├── Dashboard.py
| ├── installer.sh
| └── requirements.txt
├── Monitoramento/
│ └── ESP8266.ino
├── Docs/
| └── Manual_IC.pdf
└── README.md
```

- `Coletor/` - Contém os scripts responsáveis pela instalação do ambiente e pela coleta automatizada das leituras disponibilizadas pelo ESP8266.
- `Monitoramento/` - Contém o código embarcado responsável pela leitura dos sensores, comunicação com o ESP8266 e disponibilização dos dados através do servidor web.
- `Docs/` - Manual guia para montagem e configuração do sistema.

---

## Tecnologias utilizadas

- **Arduino IDE/Linguagem C++**
- **Python**
- **Pandas**
- **Coleta por Json**
- **Shell Script**
- **Crontab para automação**
- **Streamlit para dashboard web**
- **Plotly para gráfico interativo**

## Como configurar todo o sistema

Tudo o que é necessário saber, desde os materiais necessários, montagem dos componentes, escrita do código de cada sensor, código completo, até a instalação e execução do coletor e dashboard, estão presentes no manual disponibilizado.

- 📘 [Baixar Manual](Docs/Manual_IC.pdf)

---

## Objetivos
- **Monitorar variáveis ambientais**
- **Gerar dados estruturados para análise estatística**
- **Facilitação na medição e visualização de mudanças no ambiente**
- **Automação no processo de aquisição de dados**
- **Criar solução de baixo custo e replicável**

---

## Autor

**Orientador** - `Professor Acir Moreno Soares Junior`

**Aluno** - `Arthur Carvalho Rodrigues Oliveira`

Graduando em Ciência da Computação - Universidade Federal de São João Del-rei

Projeto de Iniciação Científica (2025-2026)

---

## Licença

Este projeto está licenciado sob a MIT License — Sinta-se à vontade para utilizar, modificar e adaptar para fins experimentais e acadêmicos.
