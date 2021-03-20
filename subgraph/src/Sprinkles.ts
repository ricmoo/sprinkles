import { Address, BigInt } from '@graphprotocol/graph-ts'
import { Sprinkle as SprinkleEntity, Owner as OwnerEntity } from '../generated/schema'
import { Transfer as TransferEvent, 
         RegisterSprinkle as RegisterEvent 
       } from '../generated/Sprinkles/Sprinkles'

export function handleRegistered(event: RegisterEvent): void {
  let sprinkle = loadOrCreateSprinkle(event.params.tokenId)
  sprinkle.publicKeyHash = event.params.publicKeyHash
  sprinkle.save()
}

export function handleTransfer(event: TransferEvent): void {
 
  let sprinkle = loadOrCreateSprinkle(event.params.tokenId)
  let owner = loadOrCreateOwner(event.params.to)
  
  sprinkle.owner = owner.id
  sprinkle.save()
  owner.save()
}

export function loadOrCreateSprinkle(tokenId: BigInt): SprinkleEntity {
  let id = tokenId.toHex()
  
  let sprinkle = SprinkleEntity.load(id)
  if (sprinkle === null) {
    sprinkle = new SprinkleEntity(id)
  }
  return sprinkle!
}

export function loadOrCreateOwner(ownerAdress: Address): OwnerEntity {
  let id = ownerAdress.toHex();

  let owner = OwnerEntity.load(id)
  if (owner === null) {
    owner = new OwnerEntity(id)
    owner.address = ownerAdress
  }
  return owner!
}

