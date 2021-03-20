let address;
const subgraphUrl = 'https://api.thegraph.com/subgraphs/name/yuetloo/sprinkle-v0-ropsten'
const sprinkleContractAddress = '0x2B240bdA2B9Ce6A9229E40aAbA9D1b0916D00F40';

window.onload = () => {
   const connectButton = document.getElementById('connect');
   connectButton.addEventListener('click', handleConnect);

   const homePage = document.getElementById('home');
   const dashboard = document.getElementById('dashboard');

   const addressLabel = document.getElementById('address');
   const dashboardContent = document.querySelector('#dashboard .card-container');

   const notFound = document.getElementById('not-found');

   function setAccountAddress(addr) {
      address = addr;
      addressLabel.textContent = formatAdress(addr);
   }

   function formatAdress(addr) {
      return addr.slice(0, 10) + "..." + addr.slice(-8);
   }

   function gotoDashboard() {
      homePage.style.display = 'none';
      dashboard.style.display = 'block';
      refreshDashboard();
   }

   function refreshDashboard() {
      dashboardContent.textContent =""
      showSpinner();
      fetchSprinkles(address)
         .then(listSprinkles)
         .finally(() => {
            hideSpinner();
         })
   }

   function showSpinner() {

   }

   function hideSpinner() {

   }

   function listSprinkles(sprinkles) {
      if( sprinkles.length === 0 ) {
         notFound.style="display:block";
         return;
      }

      notFound.style="display:none";
      sprinkles.forEach(sprinkle => {
         const token = {
            contractAddress: sprinkleContractAddress,
            id: sprinkle.id,
            name: "Sprinkle",
            owner: sprinkle.owner.id
         }
         dashboardContent.appendChild(buildCard(token));
      })
   }

   function buildCard(token) {
      const box = document.createElement("div");
      const titleBox = document.createElement('div');
      const idBox = document.createElement('div');

      const name = document.createTextNode(token.name);
      const idText = document.createTextNode("# " + token.id);

      box.classList.add("token-card");
      titleBox.classList.add("token-name");
      idBox.classList.add('token-id')

      console.log('token', token)
      titleBox.appendChild(name);
      idBox.appendChild(idText);

      box.appendChild(titleBox);
      box.appendChild(idBox);
      return box;
   }

   async function fetchSprinkles(owner) {
      owner = '0x236ff1e97419ae93ad80cafbaa21220c5d78fb7d'
      const json = {
         query: `
         {
            owners (where: { id: "${owner}"}) {
               id
               address
               sprinkles {
                 id
                 owner
                 publicKeyHash
               }
            }
         }
         `
       }

      const res = await ethers.utils.fetchJson(subgraphUrl, JSON.stringify(json));
      const sprinkles = res.data.owners[0]? res.data.owners[0].sprinkles : [];
      return sprinkles || [];
   }

   async function fetchAssets() {
      
      const url=`https://api.opensea.io/api/v1/assets?owner=${address}`
      //const collectionUrl = `https://api.opensea.io/api/v1/collections?asset_owner=${address}`;
      
      const result = await ethers.utils.fetchJson(url);
      return result.assets;
   }

   function handleConnect() {
      if (typeof window.ethereum === 'undefined') {
         alert('MetaMask is not installed!');
         return;
      }

      ethereum.request({ method: 'eth_requestAccounts' }).then(async accounts => {
         const provider = new ethers.providers.Web3Provider(window.ethereum);
         provider.ready.then(() => {
            if( provider.network.chainId !== 1) {
                  alert("Please connect to mainnet");
                  return;
            }
            
            ethereum.on('accountsChanged', (newAccounts) => {
               setAccountAddress(newAccounts[0]);
               refreshDashboard();
            });

            ethereum.on('chainChanged', (chainId) => {
               window.location.reload();
            });

            setAccountAddress(accounts[0]);
            gotoDashboard();

         });
      })
   }
};
