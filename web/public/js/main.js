let address;
const subgraphUrl = 'https://api.thegraph.com/subgraphs/name/yuetloo/sprinkle-v0-ropsten'
const sprinkleContractAddress = '0x2B240bdA2B9Ce6A9229E40aAbA9D1b0916D00F40';


window.onload = () => {
   const connectButton = document.getElementById('connect');
   connectButton.addEventListener('click', handleConnect);

   const homePage = document.getElementById('home');
   const showRoom = document.getElementById('show-room');
   const gallery = document.getElementById('gallery');

   const addressLabel = document.getElementById('address');

   const notFound = document.getElementById('not-found');

   function setAccountAddress(addr) {
      address = addr;
      addressLabel.textContent = formatAdress(addr);
   }

   function formatAdress(addr) {
      return addr.slice(0, 10) + "..." + addr.slice(-8);
   }

   function showNotFound(what) {
      const box = document.createElement('div');
      const message = document.createTextNode(`${what} not found`);
      box.classList.add("not-found");
      box.appendChild(message);
      gallery.appendChild(box);
   }

   function handleSave(token) {
      return () => {

      }

   }

   function showAssets(owner) {
      console.log('sprinkle clicked')
      return async (e) => {
         gallery.textContent="";
         owner = '0x23936429FC179dA0e1300644fB3B489c736D562F';
         const assets = await fetchAssets(owner);
         console.log('got assets', assets)

         if( assets.length === 0 ) {
            showNotFound("Assets");
         }

         assets.forEach(asset => {
            console.log('build card asset', asset)
            const tokenElement = buildCard(asset);
            gallery.appendChild(tokenElement);
            tokenElement.addEventListener('click', handleSave(asset));
         })
      }
   }

   function gotoDashboard() {
      homePage.style.display = 'none';
      showRoom.style.display = 'block';
      refreshDashboard();
   }

   function refreshDashboard() {
      gallery.textContent =""
      const addr = '0x236ff1e97419ae93ad80cafbaa21220c5d78fb7d';
      showSpinner();
      fetchSprinkles(addr)
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

      sprinkles.forEach(sprinkle => {
         const token = {
            contractAddress: sprinkleContractAddress,
            id: sprinkle.id,
            name: `# ${ethers.BigNumber.from(sprinkle.id).toString()}`,
            collection: "Sprinkles",
            owner: sprinkle.owner.id
         }
         const tokenElement = buildCard(token);
         gallery.appendChild(tokenElement);
         console.log('sprinkle', sprinkle, token)
         tokenElement.addEventListener('click', showAssets(token.owner));
      })
   }

   function buildCard(token) {
      const box = document.createElement("div");
      box.classList.add("token-card");

      if( token.imageUrl ) {
         console.log('build image', token.imageUrl)
         const image = document.createElement('img');
         image.src = token.imageUrl;
         image.width = "200";
         image.height = "200";
         image.alt = token.name;
         box.appendChild(image);
      }

      console.log('building...', token)
      const collection = document.createElement('div');
      const collectionText = document.createTextNode(token.collection);
      collection.appendChild(collectionText);
      collection.classList.add("collection");

      const name = document.createElement('div');
      const nameText = document.createTextNode(token.name);
      name.classList.add("token-name");
      name.appendChild(nameText)

      box.appendChild(collection)
      box.appendChild(name);
      return box;
   }

   async function fetchSprinkles(owner) {
      const json = {
         query: `
         {
            owners (where: { id: "${owner}"}) {
               id
               address
               sprinkles {
                 id
                 owner { id }
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

   async function fetchAssets(owner) {
      
      const url=`https://api.opensea.io/api/v1/assets?owner=${owner}`
      
      const result = await ethers.utils.fetchJson(url);
      const assets = result.assets.map(asset => ({
         contractAddress: asset.asset_contract.address,
         id: asset.token_id,
         name: asset.name,
         collection: asset.collection.name,
         owner,
         imageUrl: asset.image_url,
         imagePreviewUrl: asset.image_preview_url,
         imageThumbnailUrl: asset.image_thumbnail_url
      }));
      return assets;
   }

   function handleConnect() {
      if (typeof window.ethereum === 'undefined') {
         alert('MetaMask is not installed!');
         return;
      }

      ethereum.request({ method: 'eth_requestAccounts' }).then(async accounts => {
         const provider = new ethers.providers.Web3Provider(window.ethereum);
         provider.ready.then(() => {
            ethereum.on('accountsChanged', (newAccounts) => {
               setAccountAddress(newAccounts[0]);
               refreshDashboard();
            });

            setAccountAddress(accounts[0]);
            gotoDashboard();

         });
      })
   }
};
