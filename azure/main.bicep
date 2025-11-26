param access_tier string = 'Hot'
param location string = resourceGroup().location

@allowed([
  'live'
  'test'
])
param policy_definitions string = 'test'

@minLength(3)
@maxLength(24)
param storage_account_name string

param storage_sku string = 'Standard_LRS'

resource storage_account 'Microsoft.Storage/storageAccounts@2025-06-01' = {
  name: storage_account_name
  location: location
  sku: {
    name: storage_sku
  }
  kind: 'StorageV2'
  properties: {
    publicNetworkAccess: 'Enabled'
    minimumTlsVersion: 'TLS1_2'
    allowSharedKeyAccess: true
    accessTier: access_tier
  }
}

resource blob_service 'Microsoft.Storage/storageAccounts/blobServices@2025-06-01' = {
  parent: storage_account
  name: 'default'
  properties: {
    containerDeleteRetentionPolicy: {
      enabled: true
      days: 7
    }
    deleteRetentionPolicy: {
      enabled: true
      days: 7
    }
  }
}

resource container 'Microsoft.Storage/storageAccounts/blobServices/containers@2025-06-01' = {
  parent: blob_service
  name: 'backups'
  properties: {
    defaultEncryptionScope: '$account-encryption-key'
    denyEncryptionScopeOverride: false
    publicAccess: 'None'
  }
}

resource test_policies 'Microsoft.Storage/storageAccounts/managementPolicies@2025-06-01' = if (policy_definitions == 'test') {
  name: 'default'
  parent: storage_account
  properties: {
    policy: {
      rules: [
        {
          name: 'delete-old-blobs'
          enabled: true
          type: 'Lifecycle'
          definition: {
            filters: {
              blobTypes: ['blockBlob']
            }
            actions: {
              baseBlob: {
                delete: {
                  daysAfterCreationGreaterThan: 7
                }
              }
            }
          }
        }
      ]
    }
  }
}

resource policies 'Microsoft.Storage/storageAccounts/managementPolicies@2025-06-01' = if (policy_definitions == 'live') {
  name: 'default'
  parent: storage_account
  properties: {
    policy: {
      rules: [
        {
          name: 'cool-store-blobs'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              baseBlob: {
                tierToCool: {
                  daysAfterModificationGreaterThan: 7
                }
              }
            }
            filters: {
              blobTypes: ['blockBlob']
            }
          }
        }
        {
          name: 'archive-old-blobs'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              baseBlob: {
                tierToArchive: {
                  daysAfterModificationGreaterThan: 45
                }
              }
            }
            filters: {
              blobTypes: ['blockBlob']
            }
          }
        }
        {
          name: 'delete-old-blobs'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              baseBlob: {
                delete: {
                  daysAfterCreationGreaterThan: 730
                }
              }
            }
            filters: {
              blobTypes: ['blockBlob']
            }
          }
        }
      ]
    }
  }
}
