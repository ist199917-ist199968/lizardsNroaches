# TODO: 

## GENERAL

[] Disconnect users (roaches, lizards, wasps) +
[] + timeout, 1 minuto de inatividade. Caso o user queira voltar mandar notificação que as mensagens são inválidas
[X] limite de wasps + cocks

## Threading
 
[] 4 threads para lizards e 1 para cocks and wasps
[] mudar o display para lizard client
[] lizard perde quando score negativo e passa-se a desenhar apenas a cabeça

## Protobuffer

[] "The interface between the server and the Wasps-client / Roaches-client and the
server should be supported by Protocol Buffers for message encoding."
    [X] Create .proto file with new message structure
    [] Update Code to be protobuf-complaint
    []

## Extras:

[] Wasps ou Roaches com diferente linguagem (opcional)


## Wasps: DONE
[X] random spawn
[X] não pode existir numa casa com outras wasps, roaches ou cabeças de lizard
[X] quando uma wasp se move contra a cabeça dum lizard ou vice versa, diminuir o score do lizard e ambos não se movem!


## Report:
[] descrever as funcionalidades implementadas
[] descrever a arquitetura dos sistemas
[] descrever os protocolos de comunicação
[] descrição crítica das mudanças entre as duas versões
