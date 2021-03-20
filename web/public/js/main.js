let address;

window.onload = () => {
   const connectButton = document.getElementById('connect');
   connectButton.addEventListener('click', handleConnect);

   const homePage = document.getElementById('home');
   const dashboard = document.getElementById('dashboard');

   const addressLabel = document.getElementById('address');
   const dashboardContent = document.querySelector('#dashboard .content');

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
      showSpinner();
      fetchSprinkles()
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
         dashboardContent.appendChild(buildCard(sprinkle));
      })
   }

   function buildCard(token) {
      const box = document.createElement("div");
      var text = document.createTextNode("token name + collection");
      console.log('token', token)
      box.appendChild(text);
      return box;
   }

   async function fetchSprinkles() {
      const abi = [
         "event RegisterSprinkle(bytes32 indexed publicKeyHash, uint256 tokenId)",
         "event Transfer(address indexed from, address indexed to, uint256 indexed tokenId)"
       ];
       /*
       const contract = new ethers.Contract("sprinkles.eth", abi, provider);
       console.log("Registered:", await contract.queryFilter("RegisterSprinkle"));
       console.log("Owners:", await contract.queryFilter("Transfer"));
       */
      return [];
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